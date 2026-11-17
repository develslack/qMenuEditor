# qMenuEditor

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg?logo=c%2B%2B)
![Qt](https://img.shields.io/badge/Qt-5%20%7C%206-41CD52.svg?logo=qt)
![Platform](https://img.shields.io/badge/Platform-Linux%20(XDG)-lightgrey.svg?logo=linux)
![Distro](https://img.shields.io/badge/Tested%20on-Slackware-333333.svg)

**qMenuEditor** es un editor de menús y lanzadores de aplicaciones ligero, rápido y nativo para sistemas GNU/Linux, desarrollado en **C++ y Qt**.

Nace como una alternativa directa a herramientas basadas en Python/GTK (como *MenuLibre*), eliminando la sobrecarga de intérpretes y dependencias externas para depender exclusivamente de la biblioteca estándar de C++ y los módulos base de Qt.

---

## ✨ Características principales

* **Sin dependencias de Python ni GTK:** Binario nativo compilado en C++17.
* **Cumplimiento estricto del estándar Freedesktop.org (XDG):**
  * Respeta la jerarquía entre lanzadores del sistema (`/usr/share/applications/`) y personalizaciones del usuario (`~/.local/share/applications/`).
  * Preserva comentarios, estructura original y claves localizadas (`Name[es]`, `Comment[es]`) al editar archivos `.desktop`.
* **Gestión de privilegios integrada por lanzador:** Permite configurar cada entrada para ejecutarse como **usuario normal**, **root gráfico (`pkexec`)**, **`sudo` en terminal** o **`su -c` en terminal**, detectando y ensamblando los prefijos automáticamente.
* **Gestión completa de elementos:**
  * Creación, edición y prueba en vivo de lanzadores (limpiando códigos de campo XDG como `%u`, `%F`, `%i`).
  * Creación y eliminación protegida (con autenticación `root`) de categorías personalizadas (`.directory`).
  * Restauración automática de lanzadores del sistema al eliminar copias locales modificadas, u ocultamiento seguro mediante `Hidden=true`.
* **Modo dual de ejecución:** Funciona tanto como usuario sin privilegios (perfil local) como con permisos de `root` (edición global del sistema).

---

## 🖥️ Entornos de escritorio compatibles

Al adherirse a las especificaciones XDG de Freedesktop, **qMenuEditor** funciona de manera nativa en cualquier entorno gráfico o gestor de ventanas que utilice lanzadores `.desktop`:

* **XFCE** *(entorno principal de desarrollo y pruebas)*
* **KDE Plasma** *(integración visual 100 % nativa)*
* **LXQt**
* **MATE / Cinnamon**
* **Gestores de ventanas con lanzadores XDG:** Openbox, Fluxbox, i3, Sway, Hyprland (con Rofi, Wofi, jgmenu, etc.).

---

## 🛠️ Tecnologías utilizadas

| Componente | Tecnología / Estándar | Propósito |
| :--- | :--- | :--- |
| **Lenguaje** | C++17 | Lógica principal, gestión de memoria y parseo de archivos. |
| **Framework UI / Core** | Qt 5 (compatible con Qt 6) | Interfaz gráfica (`QtWidgets`), manejo de rutas (`QStandardPaths`), iconos (`QIcon::fromTheme`) y procesos (`QProcess`). |
| **Sistema de construcción** | `qmake` (`.pro`) | Generación portable de `Makefile`, compatible con Qt 5, Qt 6 y Qt Creator. |
| **Especificaciones** | Freedesktop Desktop Entry & Menu Spec | Lectura/escritura de archivos `.desktop` y `.directory`. |
| **Automatización y Deploy** | GNU Bash, `ldd`, `strip`, `tar` | Compilación, instalación y generación de instaladores autocontenidos `.run`. |

---

## 📂 Estructura del proyecto

```text
qMenuEditor/
├── build/                  # Directorio de salida para binarios, objetos e instaladores .run
├── documentation/          # Documentación técnica para desarrolladores (ES / EN)
├── resources/
│   └── icons/              # Iconos propios de la aplicación (PNG / SVG)
├── src/
│   ├── core/               # Lógica base independiente de la interfaz (DesktopEntry.h/.cpp)
│   ├── modules/            # Módulos funcionales específicos de extensión
│   ├── ui/                 # Interfaz gráfica y lógica de presentación (MainWindow.h/.cpp)
│   └── main.cpp            # Punto de entrada de la aplicación
├── build.sh                # Script maestro de compilación, instalación y despliegue
└── qMenuEditor.pro         # Archivo de proyecto para qmake y Qt Creator
```

---

## 🚀 Guía completa de uso de `build.sh`

El proyecto incluye el script `build.sh` en la raíz para automatizar todo el ciclo de vida: limpieza, compilación, ejecución, instalación en `/opt/qMenuEditor/` y empaquetado portable.

Primero, asegúrate de que tenga permisos de ejecución:

```bash
chmod +x build.sh
```

### Tabla de parámetros disponibles

Todos los parámetros son **combinables entre sí** en cualquier orden:

| Corto | Largo | Descripción |
| :---: | :--- | :--- |
| `-h` | `--help` | Muestra el menú de ayuda con las opciones disponibles y finaliza. |
| `-c` | `--clean` | Elimina el directorio `build/`, el `Makefile` y la caché de `qmake` antes de compilar desde cero. |
| `-d` | `--debug` | Configura y compila el proyecto en modo **Debug** (incluye símbolos de depuración). |
| `-r` | `--release` | Configura y compila el proyecto en modo **Release** optimizado *(modo por defecto)*. |
| `-i` | `--install` | Instala la aplicación compilada en `/opt/qMenuEditor/` y crea el enlace en `/usr/local/bin/` y su entrada en el menú del sistema. |
| — | `--make-deploy` | Fuerza compilación en **Release**, recolecta las librerías y plugins de Qt y genera un instalador autocontenido `build/qMenuEditor-<arch>.run`. |
| `-x` | `--run` | Ejecuta la aplicación inmediatamente después de finalizar la compilación (o instalación). |
| — | `--root` | Ejecuta la aplicación con privilegios de `root` mediante `pkexec`, `sudo` o `su` *(activa automáticamente `--run`)*. |

---

### Detalle de funcionamiento interno de `build.sh`

1. **Detección automática de versión de Qt:**
   El script busca automáticamente el compilador de proyectos en el siguiente orden de prioridad:
   1. `qmake-qt5` *(estándar en distribuciones como Slackware)*
   2. `qmake`
   3. `qmake6` *(para sistemas con Qt 6)*
2. **Compilación paralela:**
   Detecta el número de núcleos disponibles mediante `nproc` y ejecuta `make -j<N>` dentro de `build/` para mantener limpio el árbol de código fuente.
3. **Instalación estándar (`--install`):**
   * Copia el ejecutable en `/opt/qMenuEditor/bin/qMenuEditor`.
   * Copia los iconos de `resources/icons/` en `/opt/qMenuEditor/icons/`.
   * Crea un enlace simbólico en `/usr/local/bin/qMenuEditor`.
   * Genera e instala el lanzador `/usr/share/applications/qMenuEditor.desktop`.
4. **Empaquetado autocontenido (`--make-deploy`):**
   Crea un archivo ejecutable `build/qMenuEditor-<arquitectura>.run` (por ejemplo, `qMenuEditor-x86_64.run`) que embebe todas las librerías Qt (`libQt5*` / `libQt6*` e `icu`) y plugins necesarios (`platforms`, `platformthemes`, `iconengines`, `imageformats`, `xcbglintegrations`), permitiendo instalar **qMenuEditor** en sistemas que no tengan Qt instalado.

---

### Ejemplos prácticos de uso

* **Compilar en modo Debug desde cero y abrir la aplicación para probar cambios:**
  ```bash
  ./build.sh --clean --debug --run
  # Forma corta equivalente:
  ./build.sh -c -d -x
  ```

* **Compilar en modo Release e instalar en `/opt/qMenuEditor/`:**
  ```bash
  ./build.sh --clean --release --install
  ```

* **Compilar, instalar y lanzar inmediatamente con permisos de administrador (`root`):**
  ```bash
  ./build.sh -i --root
  ```

* **Generar el paquete instalador autocontenido `.run` para distribución:**
  ```bash
  ./build.sh --clean --make-deploy
  ```

---

## 📦 Instalación mediante el paquete `.run`

Si has generado el paquete con `--make-deploy` (o descargado una release), puedes instalarlo en cualquier equipo ejecutando:

```bash
chmod +x build/qMenuEditor-x86_64.run
./build/qMenuEditor-x86_64.run
```

El instalador solicitará permisos de `root` automáticamente y desplegará la siguiente estructura en `/opt/qMenuEditor/`:

```text
/opt/qMenuEditor/
├── bin/
│   ├── qMenuEditor         # Ejecutable compilado con RPATH=\$ORIGIN/../libs
│   └── qt.conf             # Configuración de rutas relativas para Qt
├── icons/                  # Juego de iconos de la aplicación
├── libs/                   # Bibliotecas compartidas de Qt e ICU autocontenidas
├── plugins/                # Plugins de plataforma, temas e iconos de Qt
└── qMenuEditor.desktop     # Archivo de entrada de menú XDG
```

---

## 👨‍💻 Autoría

Desarrollado por **Slackzone Development**.
