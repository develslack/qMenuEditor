#!/usr/bin/env bash
set -e

APP_NAME="qMenuEditor"
BUILD_DIR="build"
BIN_PATH="${BUILD_DIR}/${APP_NAME}"
INSTALL_DIR="/opt/${APP_NAME}"

# Valores por defecto
DO_CLEAN=false
BUILD_MODE="release"
DO_INSTALL=false
DO_DEPLOY=false
DO_RUN=false
AS_ROOT=false

# Colores para la salida en terminal
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

show_help() {
    cat << EOF
Uso: ./build.sh [OPCIONES]

Script de compilación, instalación, despliegue y ejecución para ${APP_NAME}.
Las opciones son combinables entre sí.

Opciones:
  -h, --help         Muestra esta ayuda y sale.
  -c, --clean        Limpia el directorio '${BUILD_DIR}' y Makefiles previos antes de compilar.
  -d, --debug        Compila en modo Debug (con símbolos de depuración).
  -r, --release      Compila en modo Release optimizado (por defecto).
  -i, --install      Instala la aplicación en '${INSTALL_DIR}/bin' y '${INSTALL_DIR}/icons'.
      --make-deploy  Compila en Release, empaqueta 'libs/' y 'plugins/' y genera '${BUILD_DIR}/${APP_NAME}-<arch>.run'.
  -x, --run          Ejecuta la aplicación tras compilar (o instalar).
      --root         Ejecuta la aplicación con privilegios de superusuario (implica --run).

Ejemplos:
  ./build.sh --clean --make-deploy
  ./build.sh --clean --release --install
  ./build.sh -c -d -x
EOF
}

# Parseo de argumentos combinables
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            DO_CLEAN=true
            shift
            ;;
        -d|--debug)
            BUILD_MODE="debug"
            shift
            ;;
        -r|--release)
            BUILD_MODE="release"
            shift
            ;;
        -i|--install)
            DO_INSTALL=true
            shift
            ;;
        --make-deploy)
            DO_DEPLOY=true
            BUILD_MODE="release"
            shift
            ;;
        -x|--run)
            DO_RUN=true
            shift
            ;;
        --root)
            AS_ROOT=true
            DO_RUN=true
            shift
            ;;
        *)
            echo -e "${RED}[Error] Opción desconocida: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# Si se pidió --make-deploy, aseguramos modo release
if [ "$DO_DEPLOY" = true ]; then
    BUILD_MODE="release"
fi

# Detectar qmake (prioriza Qt5 en Slackware, con fallback a qmake estándar o qmake6)
if command -v qmake-qt5 &> /dev/null; then
    QMAKE_BIN="qmake-qt5"
elif command -v qmake &> /dev/null; then
    QMAKE_BIN="qmake"
elif command -v qmake6 &> /dev/null; then
    QMAKE_BIN="qmake6"
else
    echo -e "${RED}[Error] No se encontró qmake en el sistema.${NC}"
    exit 1
fi

# 1. Limpieza previa si se solicitó
if [ "$DO_CLEAN" = true ]; then
    echo -e "${YELLOW}==> Limpiando directorio de compilación...${NC}"
    rm -rf "${BUILD_DIR}" Makefile .qmake.stash
fi

mkdir -p "${BUILD_DIR}"

# 2. Generación de Makefile con qmake
echo -e "${GREEN}==> Configurando proyecto con ${QMAKE_BIN} (Modo: ${BUILD_MODE})...${NC}"
"${QMAKE_BIN}" "${APP_NAME}.pro" "CONFIG+=${BUILD_MODE}" "PREFIX=${INSTALL_DIR}" -o "${BUILD_DIR}/Makefile"

# 3. Compilación paralela
CORES=$(nproc 2>/dev/null || echo 2)
echo -e "${GREEN}==> Compilando con ${CORES} hilos...${NC}"
make -C "${BUILD_DIR}" -j"${CORES}"

echo -e "${GREEN}==> Compilación exitosa: ${BIN_PATH}${NC}"

# Función auxiliar para generar el archivo .desktop estándar
generate_desktop_file() {
    local dest_file="$1"
    local icon_value="${INSTALL_DIR}/icons/${APP_NAME}.png"
    if [ ! -f "resources/icons/${APP_NAME}.png" ] && [ ! -f "resources/icons/${APP_NAME}.svg" ]; then
        icon_value="preferences-desktop-menu"
    elif [ -f "resources/icons/${APP_NAME}.svg" ]; then
        icon_value="${INSTALL_DIR}/icons/${APP_NAME}.svg"
    fi

    cat > "${dest_file}" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=qMenuEditor
GenericName=Editor de Menú
Comment=Editor ligero de entradas de menú XDG en Qt/C++
Exec=${INSTALL_DIR}/bin/${APP_NAME}
Icon=${icon_value}
Terminal=false
Categories=Settings;DesktopSettings;Qt;
StartupNotify=true
EOF
}

# 4. Generación de paquete autocontenido .run (--make-deploy)
if [ "$DO_DEPLOY" = true ]; then
    ARCH=$(uname -m)
    STAGE_DIR="${BUILD_DIR}/deploy_stage"
    RUN_OUTPUT="${BUILD_DIR}/${APP_NAME}-${ARCH}.run"

    echo -e "${YELLOW}==> Preparando empaquetado autocontenido para ${ARCH}...${NC}"
    rm -rf "${STAGE_DIR}"
    mkdir -p "${STAGE_DIR}/bin" "${STAGE_DIR}/libs" "${STAGE_DIR}/plugins" "${STAGE_DIR}/icons"

    # Copiar ejecutable y eliminar símbolos de depuración innecesarios
    cp "${BIN_PATH}" "${STAGE_DIR}/bin/${APP_NAME}"
    strip --strip-unneeded "${STAGE_DIR}/bin/${APP_NAME}" 2>/dev/null || true

    # Copiar iconos del proyecto si existen
    if [ -d "resources/icons" ] && [ "$(ls -A resources/icons 2>/dev/null)" ]; then
        cp -r resources/icons/* "${STAGE_DIR}/icons/"
    fi

    # Generar qt.conf apuntando a ../plugins y ../libs
    cat > "${STAGE_DIR}/bin/qt.conf" << EOF
[Paths]
Prefix = ..
Plugins = plugins
Libraries = libs
EOF

    # Recolectar librerías compartidas de Qt e ICU usadas por el ejecutable
    echo -e "${GREEN}==> Recolectando librerías Qt en libs/...${NC}"
    QT_LIBS=$(ldd "${BIN_PATH}" | awk '/=> \// {print $3}' | grep -E '/lib(Qt[56]|icu)' || true)
    for lib in ${QT_LIBS}; do
        if [ -f "${lib}" ]; then
            cp -Ln "${lib}" "${STAGE_DIR}/libs/"
        fi
    done

    # Recolectar plugins esenciales de Qt desde la instalación usada para compilar
    QT_PLUGINS_DIR=$("${QMAKE_BIN}" -query QT_INSTALL_PLUGINS)
    echo -e "${GREEN}==> Copiando plugins Qt en plugins/ desde ${QT_PLUGINS_DIR}...${NC}"
    for plugin_subdir in platforms platformthemes iconengines imageformats xcbglintegrations; do
        if [ -d "${QT_PLUGINS_DIR}/${plugin_subdir}" ]; then
            cp -r "${QT_PLUGINS_DIR}/${plugin_subdir}" "${STAGE_DIR}/plugins/"
        fi
    done

    # Asegurar que las dependencias Qt de los propios plugins también estén en libs/
    PLUGIN_LIBS=$(find "${STAGE_DIR}/plugins" -name "*.so" -exec ldd {} + 2>/dev/null | awk '/=> \// {print $3}' | grep -E '/lib(Qt[56]|icu)' | sort -u || true)
    for plib in ${PLUGIN_LIBS}; do
        if [ -f "${plib}" ]; then
            cp -Ln "${plib}" "${STAGE_DIR}/libs/"
        fi
    done

    # Eliminar símbolos innecesarios en libs/ y plugins/ para reducir tamaño
    find "${STAGE_DIR}/libs" "${STAGE_DIR}/plugins" -type f -name "*.so*" -exec strip --strip-unneeded {} + 2>/dev/null || true

    # Generar entrada de menú .desktop dentro del paquete
    generate_desktop_file "${STAGE_DIR}/${APP_NAME}.desktop"

    # Empaquetar payload tar.gz
    PAYLOAD_TAR="${BUILD_DIR}/payload.tar.gz"
    tar -czf "${PAYLOAD_TAR}" -C "${STAGE_DIR}" .

    # Construir script instalador autocontenido .run
    cat > "${RUN_OUTPUT}" << 'EOF'
#!/usr/bin/env bash
set -e

APP_NAME="qMenuEditor"
INSTALL_DIR="/opt/${APP_NAME}"

if [ "$(id -u)" -ne 0 ]; then
    echo "==> Se requieren permisos de root para instalar ${APP_NAME} en ${INSTALL_DIR}..."
    if command -v sudo &> /dev/null; then
        exec sudo bash "$0" "$@"
    elif command -v pkexec &> /dev/null; then
        exec pkexec bash "$0" "$@"
    else
        exec su -c "bash '$0' $*"
    fi
fi

echo "==> Instalando ${APP_NAME} en ${INSTALL_DIR}..."
rm -rf "${INSTALL_DIR}"
mkdir -p "${INSTALL_DIR}"

ARCHIVE_LINE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0;}' "$0")
tail -n+"${ARCHIVE_LINE}" "$0" | tar -xzf - -C "${INSTALL_DIR}"

chmod 755 "${INSTALL_DIR}/bin/${APP_NAME}"
ln -sf "${INSTALL_DIR}/bin/${APP_NAME}" "/usr/local/bin/${APP_NAME}"

if [ -f "${INSTALL_DIR}/${APP_NAME}.desktop" ]; then
    mkdir -p /usr/share/applications
    cp "${INSTALL_DIR}/${APP_NAME}.desktop" "/usr/share/applications/${APP_NAME}.desktop"
fi

echo "==> ${APP_NAME} instalado correctamente en ${INSTALL_DIR}."
echo "==> Puedes ejecutarlo desde el menú de aplicaciones o escribiendo: ${APP_NAME}"
exit 0

__ARCHIVE_BELOW__
EOF

    cat "${PAYLOAD_TAR}" >> "${RUN_OUTPUT}"
    chmod +x "${RUN_OUTPUT}"

    # Limpiar archivos temporales del staging
    rm -rf "${STAGE_DIR}" "${PAYLOAD_TAR}"

    echo -e "${GREEN}==> Paquete autocontenido generado: ${RUN_OUTPUT} ($(du -h "${RUN_OUTPUT}" | cut -f1))${NC}"
fi

# 5. Instalación directa en /opt/qMenuEditor/ si se solicitó --install (y no --make-deploy)
if [ "$DO_INSTALL" = true ] && [ "$DO_DEPLOY" = false ]; then
    echo -e "${YELLOW}==> Instalando ${APP_NAME} en ${INSTALL_DIR}/...${NC}"
    TMP_DESKTOP="${BUILD_DIR}/${APP_NAME}.desktop"
    generate_desktop_file "${TMP_DESKTOP}"

    INSTALL_CMD="mkdir -p '${INSTALL_DIR}/bin' '${INSTALL_DIR}/icons' && \
                 make -C '${BUILD_DIR}' install && \
                 ln -sf '${INSTALL_DIR}/bin/${APP_NAME}' '/usr/local/bin/${APP_NAME}' && \
                 cp '${TMP_DESKTOP}' '/usr/share/applications/${APP_NAME}.desktop'"

    if [ "$(id -u)" -eq 0 ]; then
        eval "${INSTALL_CMD}"
    elif command -v sudo &> /dev/null; then
        sudo bash -c "${INSTALL_CMD}"
    elif command -v pkexec &> /dev/null; then
        pkexec bash -c "${INSTALL_CMD}"
    else
        echo -e "${YELLOW}Introduce la contraseña de root para instalar en ${INSTALL_DIR}:${NC}"
        su -c "${INSTALL_CMD}"
    fi

    rm -f "${TMP_DESKTOP}"
    echo -e "${GREEN}==> Instalación completada en ${INSTALL_DIR}/bin/${APP_NAME}${NC}"
fi

# 6. Ejecución si se indicó --run o --root
if [ "$DO_RUN" = true ]; then
    if [ "$DO_INSTALL" = true ] && [ "$DO_DEPLOY" = false ]; then
        EXEC_TARGET="${INSTALL_DIR}/bin/${APP_NAME}"
    else
        EXEC_TARGET="./${BIN_PATH}"
    fi

    if [ "$AS_ROOT" = true ]; then
        echo -e "${YELLOW}==> Ejecutando ${EXEC_TARGET} como root...${NC}"
        if [ "$(id -u)" -eq 0 ]; then
            "${EXEC_TARGET}"
        elif command -v pkexec &> /dev/null; then
            pkexec env DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" "$(realpath "${EXEC_TARGET}")"
        elif command -v sudo &> /dev/null; then
            sudo -E "${EXEC_TARGET}"
        else
            su -c "${EXEC_TARGET}"
        fi
    else
        echo -e "${GREEN}==> Ejecutando ${EXEC_TARGET}...${NC}"
        "${EXEC_TARGET}"
    fi
fi
