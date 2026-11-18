#include "AboutDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSysInfo>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QIcon>
#include <QRegularExpression>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Acerca de qMenuEditor");
    resize(660, 520);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QTabWidget *tabWidget = new QTabWidget(this);

    QString distroTabTitle = "Distribución";
    QWidget *systemTab = createSystemTab();
    QWidget *aboutTab = createAboutTab();
    QWidget *creditsTab = createCreditsTab();
    QWidget *distroTab = createDistroTab(distroTabTitle);
    QWidget *copyrightTab = createCopyrightTab();

    tabWidget->addTab(systemTab, "Sistema");
    tabWidget->addTab(aboutTab, "Acerca de");
    tabWidget->addTab(creditsTab, "Créditos");
    tabWidget->addTab(distroTab, distroTabTitle);
    tabWidget->addTab(copyrightTab, "Derechos de autor");

    mainLayout->addWidget(tabWidget, 1);

    // Botón inferior Cerrar (igual que en la captura)
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch();

    QPushButton *closeBtn = new QPushButton(QIcon::fromTheme("window-close"), "Cerrar", this);
    closeBtn->setDefault(true);
    bottomLayout->addWidget(closeBtn);
    mainLayout->addLayout(bottomLayout);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

QMap<QString, QString> AboutDialog::readOsRelease() const
{
    QMap<QString, QString> data;
    QFile file("/etc/os-release");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return data;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        int eq = line.indexOf('=');
        if (eq > 0) {
            QString key = line.left(eq).trimmed();
            QString val = line.mid(eq + 1).trimmed();
            if (val.startsWith('"') && val.endsWith('"') && val.length() >= 2) {
                val = val.mid(1, val.length() - 2);
            }
            data.insert(key, val);
        }
    }
    return data;
}

QString AboutDialog::detectCpuModel() const
{
    QFile file("/proc/cpuinfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QSysInfo::currentCpuArchitecture();
    }

    QString modelName;
    int cores = 0;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("model name")) {
            if (modelName.isEmpty()) {
                modelName = line.section(':', 1).trimmed();
            }
            cores++;
        }
    }

    if (modelName.isEmpty()) {
        return QSysInfo::currentCpuArchitecture();
    }
    return cores > 0 ? QString("%1 x %2").arg(modelName).arg(cores) : modelName;
}

QString AboutDialog::detectTotalMemory() const
{
    QFile file("/proc/meminfo");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "Desconocida";
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("MemTotal:")) {
            QString kbStr = line.section(':', 1).trimmed().section(' ', 0, 0);
            bool ok = false;
            double kb = kbStr.toDouble(&ok);
            if (ok) {
                double gib = kb / (1024.0 * 1024.0);
                return QString::number(gib, 'f', 1).replace('.', ',') + " GiB";
            }
        }
    }
    return "Desconocida";
}

QString AboutDialog::detectGpuModel() const
{
    QProcess proc;
    proc.start("lspci", QStringList());
    if (!proc.waitForFinished(1500)) {
        return "No detectada";
    }

    const QString output = QString::fromUtf8(proc.readAllStandardOutput());
    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.contains("VGA compatible controller", Qt::CaseInsensitive)
            || line.contains("3D controller", Qt::CaseInsensitive)
            || line.contains("Display controller", Qt::CaseInsensitive)) {
            QString gpu = line.section(':', 2).trimmed();
            // Limpiar sufijos de revisión entre paréntesis como "(rev 06)"
            gpu.remove(QRegularExpression("\\s*\\(rev [^)]+\\)$"));
            if (!gpu.isEmpty()) return gpu;
        }
    }
    return "No detectada";
}

QString AboutDialog::detectDesktopEnvironment() const
{
    QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (desktop.isEmpty()) {
        desktop = qEnvironmentVariable("DESKTOP_SESSION");
    }
    if (desktop.isEmpty()) {
        return "Entorno XDG estándar";
    }

    // Si es XFCE, intentar obtener versión exacta con xfce4-session --version
    if (desktop.contains("XFCE", Qt::CaseInsensitive)) {
        QProcess proc;
        proc.start("xfce4-session", QStringList() << "--version");
        if (proc.waitForFinished(1000)) {
            QString firstLine = QString::fromUtf8(proc.readAllStandardOutput()).section('\n', 0, 0);
            QRegularExpression re("(\\d+\\.\\d+(?:\\.\\d+)?)");
            auto match = re.match(firstLine);
            if (match.hasMatch()) {
                return "Xfce " + match.captured(1);
            }
        }
    }
    return desktop;
}

QWidget *AboutDialog::createSystemTab()
{
    QWidget *tab = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout(tab);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(24);

    // Icono de monitor a la izquierda (igual que en captura_1)
    QLabel *computerIcon = new QLabel(tab);
    computerIcon->setPixmap(QIcon::fromTheme("computer", QIcon::fromTheme("video-display")).pixmap(96, 96));
    computerIcon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    layout->addWidget(computerIcon);

    // Tabla de especificaciones a la derecha
    QMap<QString, QString> osInfo = readOsRelease();
    QString osPretty = osInfo.value("PRETTY_NAME", QSysInfo::prettyProductName());
    QString distroName = osInfo.value("NAME", "GNU/Linux");
    QString archBits = (QSysInfo::WordSize == 64) ? "64 bit" : "32 bit";

    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(10);

    auto addInfoRow = [&](const QString &labelText, const QString &valueText) {
        QLabel *lbl = new QLabel(labelText, tab);
        lbl->setStyleSheet("color: #666666;");
        QLabel *val = new QLabel(valueText, tab);
        val->setTextInteractionFlags(Qt::TextSelectableByMouse);
        form->addRow(lbl, val);
    };

    addInfoRow("Dispositivo", QSysInfo::machineHostName());
    addInfoRow("Nombre del sistema operativo", osPretty);
    addInfoRow("Tipo de sistema operativo", archBits);
    addInfoRow("Entorno de escritorio", detectDesktopEnvironment());
    addInfoRow("Distribuidor", distroName);
    addInfoRow("Kernel Linux", QSysInfo::kernelVersion());
    addInfoRow("Procesador", detectCpuModel());
    addInfoRow("Memoria", detectTotalMemory());
    addInfoRow("GPU", detectGpuModel());
    addInfoRow("Versión de Qt", QString::fromLatin1(qVersion()));

    layout->addLayout(form, 1);
    return tab;
}

QWidget *AboutDialog::createAboutTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    // Cabecera con logotipo y versión
    QHBoxLayout *header = new QHBoxLayout();
    QLabel *logo = new QLabel(tab);
    QIcon appIcon = QFileInfo::exists("/opt/qMenuEditor/icons/qMenuEditor.png")
            ? QIcon("/opt/qMenuEditor/icons/qMenuEditor.png")
            : QIcon::fromTheme("preferences-desktop-menu");
    logo->setPixmap(appIcon.pixmap(64, 64));

    QLabel *titleLbl = new QLabel(
        QString("<h2>qMenuEditor <small style='color:#666;'>v%1</small></h2>"
                "<p>Editor ligero y nativo de menús y lanzadores de aplicaciones XDG en C++ y Qt.</p>")
            .arg(QCoreApplication::applicationVersion()),
        tab
    );
    titleLbl->setWordWrap(true);

    header->addWidget(logo);
    header->addSpacing(12);
    header->addWidget(titleLbl, 1);
    layout->addLayout(header);

    QLabel *introLbl = new QLabel(
        "qMenuEditor permite gestionar las entradas del menú de aplicaciones sin dependencias de Python ni GTK, "
        "siendo compatible con XFCE, KDE Plasma, LXQt, MATE y cualquier entorno basado en Freedesktop.org.\n\n"
        "Los siguientes módulos forman parte del núcleo de qMenuEditor:",
        tab
    );
    introLbl->setWordWrap(true);
    layout->addWidget(introLbl);

    // Lista de componentes internos (estilo captura_2)
    auto addComponentRow = [&](const QString &iconName, const QString &title, const QString &desc) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *ic = new QLabel(tab);
        ic->setPixmap(QIcon::fromTheme(iconName).pixmap(32, 32));
        ic->setAlignment(Qt::AlignTop);

        QLabel *txt = new QLabel(QString("<b>%1</b><br>%2").arg(title, desc), tab);
        txt->setWordWrap(true);

        row->addWidget(ic);
        row->addSpacing(10);
        row->addWidget(txt, 1);
        layout->addLayout(row);
    };

    addComponentRow("document-properties", "Motor XDG DesktopEntry (src/core)",
                    "Lectura y escritura limpia de archivos .desktop y .directory preservando comentarios e idiomas.");
    addComponentRow("tools-check-spelling", "Auditor de Menú (src/modules)",
                    "Detección automática de lanzadores huérfanos y entradas duplicadas en el sistema.");
    addComponentRow("dialog-password", "Gestor de Privilegios",
                    "Soporte integrado para ejecución como usuario normal, pkexec, sudo y su -c.");
    addComponentRow("preferences-desktop-icons", "Selector de Iconos del Sistema",
                    "Exploración rápida del tema de iconos activo y selección de imágenes externas.");

    layout->addStretch();
    return tab;
}

QWidget *AboutDialog::createCreditsTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(14, 14, 14, 14);

    QTextBrowser *creditsView = new QTextBrowser(tab);
    creditsView->setOpenExternalLinks(true);
    creditsView->setHtml(
        "<h3>Desarrolladores principales</h3>"
        "<p style='margin-left:14px;'>"
        "<b>Slackzone Development</b> &lt;<a href='mailto:develslack@gmail.com'>develslack@gmail.com</a>&gt;<br>"
        "Arquitectura C++/Qt, diseño de interfaz y empaquetado para Slackware Linux."
        "</p>"
        "<h3>Repositorio y soporte</h3>"
        "<p style='margin-left:14px;'>"
        "Proyecto oficial en GitHub: <a href='https://github.com/develslack/qMenuEditor.git'>github.com/develslack/qMenuEditor</a>"
        "</p>"
        "<h3>Estándares y agradecimientos</h3>"
        "<p style='margin-left:14px;'>"
        "Basado en las especificaciones de <b>Freedesktop.org (XDG)</b> y el framework multiplataforma <b>Qt</b>.<br>"
        "Inspirado en la simplicidad y solidez de <b>Slackware Linux</b> y el entorno de escritorio <b>Xfce</b>."
        "</p>"
    );

    layout->addWidget(creditsView);
    return tab;
}

QWidget *AboutDialog::createDistroTab(QString &outDistroTabTitle)
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(14, 14, 14, 14);

    QMap<QString, QString> osInfo = readOsRelease();
    QString distroId = osInfo.value("ID", "linux").toLower();
    QString distroName = osInfo.value("NAME", "GNU/Linux");
    QString distroPretty = osInfo.value("PRETTY_NAME", distroName);
    QString homeUrl = osInfo.value("HOME_URL", "https://www.kernel.org/");

    outDistroTabTitle = distroName;

    QTextBrowser *distroView = new QTextBrowser(tab);
    distroView->setOpenExternalLinks(true);

    if (distroId.contains("slackware")) {
        distroView->setPlainText(
            QString("Sistema instalado: %1\n\n"
                    "La versión oficial de Slackware Linux, creada por Patrick Volkerding, es un sistema operativo "
                    "Linux avanzado diseñado con dos objetivos principales como máxima prioridad: facilidad de uso y "
                    "estabilidad. Al incluir el software más reciente y popular manteniendo un fuerte sentido de "
                    "tradición, simplicidad y potencia, Slackware ofrece lo mejor de todos los mundos.\n\n"
                    "Desarrollado originalmente por Linus Torvalds en 1991, el sistema operativo Linux tipo UNIX "
                    "cuenta hoy con las contribuciones de millones de usuarios y desarrolladores alrededor del mundo. "
                    "Slackware Linux proporciona tanto a usuarios nuevos como experimentados un sistema completo, "
                    "preparado para funcionar desde una estación de trabajo de escritorio hasta un servidor de producción, "
                    "incluyendo herramientas completas de desarrollo y compilación listas para usar.\n\n"
                    "Desde su primera versión beta en abril de 1993, el proyecto Slackware Linux ha mantenido el "
                    "compromiso de ofrecer la distribución Linux más fiel a la filosofía UNIX estándar.\n\n"
                    "Más información en: http://www.slackware.com/")
                .arg(distroPretty)
        );
    } else {
        distroView->setPlainText(
            QString("Sistema instalado: %1\n\n"
                    "qMenuEditor se encuentra ejecutándose sobre %2, un sistema operativo GNU/Linux compatible con "
                    "las especificaciones estándar de menús y lanzadores de escritorio de Freedesktop.org (XDG).\n\n"
                    "Más información sobre esta distribución en: %3")
                .arg(distroPretty, distroName, homeUrl)
        );
    }

    layout->addWidget(distroView);
    return tab;
}

QWidget *AboutDialog::createCopyrightTab()
{
    QWidget *tab = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);

    QTextBrowser *copyrightText = new QTextBrowser(tab);
    copyrightText->setPlainText(
        "qMenuEditor está registrado a nombre de Slackzone Development.\n\n"
        "Este programa se distribuye bajo los términos de la Licencia Pública General de GNU (GPL) versión 2, "
        "tal como fue publicada por la Free Software Foundation; ya sea la versión 2 de la Licencia o "
        "(a su elección) cualquier versión posterior.\n\n"
        "Este programa se distribuye con la esperanza de que sea útil, pero SIN NINGUNA GARANTÍA; "
        "ni siquiera la garantía implícita de COMERCIABILIDAD o IDONEIDAD PARA UN PROPÓSITO PARTICULAR. "
        "Consulte los textos completos de las licencias mediante los botones inferiores."
    );
    layout->addWidget(copyrightText, 1);

    // Tres botones inferiores centrados: GPL, LGPL, BSD (igual que en captura_5)
    QHBoxLayout *licenseBtnsLayout = new QHBoxLayout();
    licenseBtnsLayout->addStretch();

    QPushButton *gplBtn = new QPushButton("GPL", tab);
    QPushButton *lgplBtn = new QPushButton("LGPL", tab);
    QPushButton *bsdBtn = new QPushButton("BSD", tab);

    gplBtn->setMinimumWidth(90);
    lgplBtn->setMinimumWidth(90);
    bsdBtn->setMinimumWidth(90);

    licenseBtnsLayout->addWidget(gplBtn);
    licenseBtnsLayout->addWidget(lgplBtn);
    licenseBtnsLayout->addWidget(bsdBtn);
    licenseBtnsLayout->addStretch();

    layout->addLayout(licenseBtnsLayout);

    connect(gplBtn, &QPushButton::clicked, this, &AboutDialog::onShowGplClicked);
    connect(lgplBtn, &QPushButton::clicked, this, &AboutDialog::onShowLgplClicked);
    connect(bsdBtn, &QPushButton::clicked, this, &AboutDialog::onShowBsdClicked);

    return tab;
}

QString AboutDialog::readLicenseFile(const QString &fileName) const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidatePaths = {
        appDir + "/../documentation/" + fileName,
        appDir + "/documentation/" + fileName,
        "/opt/qMenuEditor/documentation/" + fileName,
        QDir::currentPath() + "/documentation/" + fileName
    };

    for (const QString &path : candidatePaths) {
        QFile file(path);
        if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            in.setCodec("UTF-8");
#endif
            return in.readAll();
        }
    }

    return QString("No se encontró el archivo de licencia '%1' en el directorio 'documentation/'.\n\n"
                   "Rutas verificadas:\n- %2")
            .arg(fileName, candidatePaths.join("\n- "));
}

void AboutDialog::showLicenseDialog(const QString &title, const QString &fileName)
{
    QDialog dlg(this);
    dlg.setWindowTitle(title);
    dlg.resize(640, 460);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QPlainTextEdit *textEdit = new QPlainTextEdit(&dlg);
    textEdit->setReadOnly(true);
    textEdit->setPlainText(readLicenseFile(fileName));
    layout->addWidget(textEdit, 1);

    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();
    QPushButton *closeBtn = new QPushButton("Cerrar", &dlg);
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void AboutDialog::onShowGplClicked()
{
    showLicenseDialog("Licencia Pública General de GNU v2 (GPLv2)", "gpl2.txt");
}

void AboutDialog::onShowLgplClicked()
{
    showLicenseDialog("Licencia Pública General Reducida de GNU (LGPL)", "lgpl.txt");
}

void AboutDialog::onShowBsdClicked()
{
    showLicenseDialog("Licencia BSD", "bsd.txt");
}
