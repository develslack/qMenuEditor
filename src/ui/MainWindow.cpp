#include "MainWindow.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTreeWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QMessageBox>
#include <QInputDialog>
#include <QStandardPaths>
#include <QDirIterator>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QDateTime>
#include <QIcon>
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    loadApplications();
}

void MainWindow::setupUi()
{
    setWindowTitle("qMenuEditor - Editor de Menú XDG");
    resize(960, 640);

    // Barra de herramientas superior
    QToolBar *toolbar = addToolBar("Acciones");
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QAction *newLauncherAct = toolbar->addAction(QIcon::fromTheme("document-new"), "Nuevo lanzador");
    QAction *newCategoryAct = toolbar->addAction(QIcon::fromTheme("folder-new"), "Nueva categoría");
    m_deleteAct = toolbar->addAction(QIcon::fromTheme("edit-delete"), "Eliminar");
    m_deleteAct->setEnabled(false);

    toolbar->addSeparator();
    QAction *reloadAct = toolbar->addAction(QIcon::fromTheme("view-refresh"), "Recargar");

    connect(newLauncherAct, &QAction::triggered, this, &MainWindow::onNewLauncherClicked);
    connect(newCategoryAct, &QAction::triggered, this, &MainWindow::onNewCategoryClicked);
    connect(m_deleteAct, &QAction::triggered, this, &MainWindow::onDeleteClicked);
    connect(reloadAct, &QAction::triggered, this, &MainWindow::onReloadClicked);

    // Contenedor principal dividido
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(splitter);

    // Panel izquierdo: buscador + árbol de categorías
    QWidget *leftPanel = new QWidget(splitter);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(6, 6, 6, 6);

    m_searchEdit = new QLineEdit(leftPanel);
    m_searchEdit->setPlaceholderText("Filtrar aplicaciones...");
    m_searchEdit->setClearButtonEnabled(true);
    leftLayout->addWidget(m_searchEdit);

    m_appTree = new QTreeWidget(leftPanel);
    m_appTree->setHeaderLabel("Categorías y Aplicaciones");
    m_appTree->setAnimated(true);
    leftLayout->addWidget(m_appTree);

    // Panel derecho: formulario de edición
    QWidget *rightPanel = new QWidget(splitter);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(12, 12, 12, 12);

    // Cabecera con vista previa de icono y nombre
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_iconPreview = new QLabel(rightPanel);
    m_iconPreview->setFixedSize(48, 48);
    m_iconPreview->setScaledContents(true);
    m_iconPreview->setFrameShape(QFrame::StyledPanel);

    m_nameEdit = new QLineEdit(rightPanel);
    m_nameEdit->setPlaceholderText("Nombre de la aplicación");
    headerLayout->addWidget(m_iconPreview);
    headerLayout->addWidget(m_nameEdit);
    rightLayout->addLayout(headerLayout);

    QFormLayout *formLayout = new QFormLayout();
    m_commentEdit = new QLineEdit(rightPanel);
    m_execEdit = new QLineEdit(rightPanel);
    m_execEdit->setPlaceholderText("Ej: /usr/bin/gparted");

    m_runModeCombo = new QComboBox(rightPanel);
    m_runModeCombo->addItem(QIcon::fromTheme("user-identity"), "Usuario normal", RunAsUser);
    m_runModeCombo->addItem(QIcon::fromTheme("dialog-password"), "Root gráfico (pkexec)", RunAsPkexec);
    m_runModeCombo->addItem(QIcon::fromTheme("utilities-terminal"), "Administrador con sudo (terminal)", RunAsSudo);
    m_runModeCombo->addItem(QIcon::fromTheme("utilities-terminal"), "Superusuario con su -c (terminal)", RunAsSu);

    m_iconEdit = new QLineEdit(rightPanel);
    m_pathEdit = new QLineEdit(rightPanel);
    m_categoriesEdit = new QLineEdit(rightPanel);
    m_terminalCheck = new QCheckBox("Ejecutar en una terminal", rightPanel);
    m_noDisplayCheck = new QCheckBox("Ocultar en el menú (NoDisplay)", rightPanel);

    formLayout->addRow("Descripción:", m_commentEdit);
    formLayout->addRow("Comando (Exec):", m_execEdit);
    formLayout->addRow("Ejecutar como:", m_runModeCombo);
    formLayout->addRow("Icono:", m_iconEdit);
    formLayout->addRow("Directorio de trabajo:", m_pathEdit);
    formLayout->addRow("Categorías (XDG):", m_categoriesEdit);
    formLayout->addRow("", m_terminalCheck);
    formLayout->addRow("", m_noDisplayCheck);

    rightLayout->addLayout(formLayout);
    rightLayout->addStretch();

    // Botonera inferior: Probar lanzador + Guardar cambios
    QHBoxLayout *bottomButtonsLayout = new QHBoxLayout();
    bottomButtonsLayout->addStretch();

    m_testButton = new QPushButton(QIcon::fromTheme("media-playback-start"), "Probar lanzador", rightPanel);
    m_testButton->setEnabled(false);

    m_saveButton = new QPushButton(QIcon::fromTheme("document-save"), "Guardar cambios", rightPanel);
    m_saveButton->setEnabled(false);

    bottomButtonsLayout->addWidget(m_testButton);
    bottomButtonsLayout->addWidget(m_saveButton);
    rightLayout->addLayout(bottomButtonsLayout);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    // Barra de estado indicando el modo de permisos
    if (geteuid() == 0) {
        statusBar()->showMessage("Modo ROOT activo: los cambios se guardarán directamente en el sistema (/usr/share/applications).");
    } else {
        statusBar()->showMessage("Modo Usuario: los cambios se guardarán en ~/.local/share/applications.");
    }

    // Conexiones
    connect(m_appTree, &QTreeWidget::currentItemChanged, this, &MainWindow::onItemSelected);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_iconEdit, &QLineEdit::textChanged, this, &MainWindow::onIconTextChanged);
    connect(m_runModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onRunModeChanged);
    connect(m_testButton, &QPushButton::clicked, this, &MainWindow::onTestLauncherClicked);
    connect(m_saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
}

void MainWindow::clearEditorForm()
{
    m_currentKey.clear();
    m_nameEdit->clear();
    m_commentEdit->clear();
    m_execEdit->clear();
    m_iconEdit->clear();
    m_pathEdit->clear();
    m_categoriesEdit->clear();
    m_terminalCheck->setChecked(false);
    m_noDisplayCheck->setChecked(false);
    m_iconPreview->clear();
    m_testButton->setEnabled(false);
    m_saveButton->setEnabled(false);
}

void MainWindow::loadApplications()
{
    m_appTree->clear();
    m_entries.clear();
    m_customCategoryNames.clear();
    clearEditorForm();
    if (m_deleteAct) m_deleteAct->setEnabled(false);

    const QStringList appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString &dir : appDirs) {
        QDirIterator it(dir, QStringList() << "*.desktop", QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString path = it.next();
            QString fileName = QFileInfo(path).fileName();

            if (!m_entries.contains(fileName)) {
                DesktopEntry entry(path);
                // Omitir entradas marcadas como Hidden=true
                if (entry.isValid() && !entry.name().isEmpty()
                    && entry.value("Hidden").compare("true", Qt::CaseInsensitive) != 0) {
                    m_entries.insert(fileName, entry);
                }
            }
        }
    }

    QMap<QString, QTreeWidgetItem *> categoryNodes;
    const QStringList mainCats = {
        "Accesorios", "Desarrollo", "Educación", "Gráficos",
        "Internet", "Multimedia", "Oficina", "Sistema", "Otros"
    };

    for (const QString &cat : mainCats) {
        auto *catItem = new QTreeWidgetItem(m_appTree, QStringList() << cat);
        catItem->setIcon(0, QIcon::fromTheme("folder"));
        catItem->setData(0, Qt::UserRole + 1, categoryToXdgTag(cat));
        categoryNodes.insert(cat, catItem);
    }

    // Cargar categorías personalizadas (.directory)
    loadCustomCategories(categoryNodes);

    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        const DesktopEntry &entry = it.value();
        QString mainCat = mapMainCategory(entry.categories(), categoryNodes);
        QTreeWidgetItem *parentNode = categoryNodes.value(mainCat, categoryNodes.value("Otros"));

        auto *appItem = new QTreeWidgetItem(parentNode, QStringList() << entry.name());
        appItem->setData(0, Qt::UserRole, it.key());

        QIcon icon = QIcon::fromTheme(entry.icon(), QIcon::fromTheme("application-x-executable"));
        appItem->setIcon(0, icon);
    }

    m_appTree->sortItems(0, Qt::AscendingOrder);
}

void MainWindow::loadCustomCategories(QMap<QString, QTreeWidgetItem *> &categoryNodes)
{
    QStringList dirLocations;
    dirLocations << QDir::homePath() + "/.local/share/desktop-directories"
                 << "/usr/local/share/desktop-directories"
                 << "/usr/share/desktop-directories";

    for (const QString &dirPath : dirLocations) {
        QDir dir(dirPath);
        if (!dir.exists()) continue;

        const QStringList files = dir.entryList(QStringList() << "qmenueditor-*.directory", QDir::Files);
        for (const QString &fileName : files) {
            QString fullPath = dir.filePath(fileName);
            DesktopEntry dirEntry(fullPath);
            QString catName = dirEntry.name();
            if (catName.isEmpty() || categoryNodes.contains(catName)) continue;

            QString xdgTag = QFileInfo(fileName).completeBaseName();
            xdgTag.remove("qmenueditor-");

            auto *catItem = new QTreeWidgetItem(m_appTree, QStringList() << catName);
            catItem->setIcon(0, QIcon::fromTheme(dirEntry.icon(), QIcon::fromTheme("folder")));
            catItem->setData(0, Qt::UserRole + 1, xdgTag);
            catItem->setData(0, Qt::UserRole + 2, fullPath); // Ruta real del archivo .directory

            categoryNodes.insert(catName, catItem);
            m_customCategoryNames.insert(xdgTag, catName);
        }
    }
}

QString MainWindow::mapMainCategory(const QStringList &categories, const QMap<QString, QTreeWidgetItem *> &categoryNodes) const
{
    for (const QString &cat : categories) {
        if (m_customCategoryNames.contains(cat)) {
            QString displayName = m_customCategoryNames.value(cat);
            if (categoryNodes.contains(displayName)) {
                return displayName;
            }
        }
    }

    if (categories.contains("AudioVideo") || categories.contains("Audio") || categories.contains("Video"))
        return "Multimedia";
    if (categories.contains("Development"))
        return "Desarrollo";
    if (categories.contains("Education"))
        return "Educación";
    if (categories.contains("Graphics"))
        return "Gráficos";
    if (categories.contains("Network"))
        return "Internet";
    if (categories.contains("Office"))
        return "Oficina";
    if (categories.contains("System") || categories.contains("Settings"))
        return "Sistema";
    if (categories.contains("Utility"))
        return "Accesorios";
    return "Otros";
}

QString MainWindow::categoryToXdgTag(const QString &displayName) const
{
    if (displayName == "Multimedia") return "AudioVideo";
    if (displayName == "Desarrollo") return "Development";
    if (displayName == "Educación")  return "Education";
    if (displayName == "Gráficos")   return "Graphics";
    if (displayName == "Internet")   return "Network";
    if (displayName == "Oficina")    return "Office";
    if (displayName == "Sistema")    return "System";
    if (displayName == "Accesorios") return "Utility";
    return "Utility";
}

void MainWindow::onNewLauncherClicked()
{
    QTreeWidgetItem *selected = m_appTree->currentItem();
    QTreeWidgetItem *targetCategoryNode = nullptr;
    QString defaultCategoryTag = "Utility";

    if (selected) {
        targetCategoryNode = selected->parent() ? selected->parent() : selected;
        QString tag = targetCategoryNode->data(0, Qt::UserRole + 1).toString();
        if (!tag.isEmpty()) {
            defaultCategoryTag = tag;
        }
    } else if (m_appTree->topLevelItemCount() > 0) {
        targetCategoryNode = m_appTree->topLevelItem(0);
    }

    QString timestamp = QString::number(QDateTime::currentSecsSinceEpoch());
    QString fileName = "custom-app-" + timestamp + ".desktop";
    QString baseDir = (geteuid() == 0)
            ? "/usr/share/applications"
            : QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QDir().mkpath(baseDir);

    DesktopEntry newEntry;
    newEntry.setName("Nueva aplicación");
    newEntry.setComment("Descripción de la aplicación");
    newEntry.setExec("");
    newEntry.setIcon("application-x-executable");
    newEntry.setCategories(QStringList() << defaultCategoryTag);
    newEntry.setTerminal(false);
    newEntry.setNoDisplay(false);

    QString fullPath = baseDir + "/" + fileName;
    newEntry.save(fullPath);

    m_entries.insert(fileName, newEntry);

    if (targetCategoryNode) {
        auto *appItem = new QTreeWidgetItem(targetCategoryNode, QStringList() << newEntry.name());
        appItem->setData(0, Qt::UserRole, fileName);
        appItem->setIcon(0, QIcon::fromTheme("application-x-executable"));
        targetCategoryNode->setExpanded(true);
        m_appTree->setCurrentItem(appItem);
    }

    m_nameEdit->setFocus();
    m_nameEdit->selectAll();
    statusBar()->showMessage("Nuevo lanzador creado. Completa los campos y pulsa Guardar.");
}

void MainWindow::onNewCategoryClicked()
{
    bool ok = false;
    QString catName = QInputDialog::getText(
        this,
        "Nueva categoría",
        "Nombre de la nueva categoría:",
        QLineEdit::Normal,
        "",
        &ok
    ).trimmed();

    if (!ok || catName.isEmpty()) return;

    QString slug = catName.normalized(QString::NormalizationForm_KD)
                          .remove(QRegularExpression("[^a-zA-Z0-9]"));
    if (slug.isEmpty()) {
        slug = "CustomCat" + QString::number(QDateTime::currentSecsSinceEpoch());
    }

    QString dirBase = (geteuid() == 0)
            ? "/usr/share/desktop-directories"
            : QDir::homePath() + "/.local/share/desktop-directories";
    QDir().mkpath(dirBase);

    QString dirFilePath = dirBase + "/qmenueditor-" + slug + ".directory";
    QFile file(dirFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, "Error", "No se pudo crear el archivo de categoría en:\n" + dirFilePath);
        return;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif
    out << "[Desktop Entry]\n"
        << "Version=1.0\n"
        << "Type=Directory\n"
        << "Name=" << catName << "\n"
        << "Icon=folder\n";
    file.close();

    auto *catItem = new QTreeWidgetItem(m_appTree, QStringList() << catName);
    catItem->setIcon(0, QIcon::fromTheme("folder"));
    catItem->setData(0, Qt::UserRole + 1, slug);
    catItem->setData(0, Qt::UserRole + 2, dirFilePath);

    m_customCategoryNames.insert(slug, catName);
    m_appTree->sortItems(0, Qt::AscendingOrder);
    m_appTree->setCurrentItem(catItem);

    statusBar()->showMessage("Categoría '" + catName + "' creada (" + dirFilePath + ").", 5000);
}

bool MainWindow::removeFileAsRoot(const QString &filePath)
{
    if (geteuid() == 0) {
        return QFile::remove(filePath);
    }

    // 1. Intentar elevación gráfica mediante pkexec (Polkit)
    if (!QStandardPaths::findExecutable("pkexec").isEmpty()) {
        int exitCode = QProcess::execute("pkexec", QStringList() << "rm" << "-f" << filePath);
        if (exitCode == 0 && !QFile::exists(filePath)) {
            return true;
        }
        // Si el usuario canceló el diálogo de contraseña (126/127), no insistimos
        if (exitCode == 126 || exitCode == 127) {
            return false;
        }
    }

    // 2. Fallback para Slackware sin agente Polkit activo: terminal con su -c
    QString term = detectTerminalEmulator();
    if (!term.isEmpty()) {
        QString escapedPath = filePath;
        escapedPath.replace('\'', "'\\''");
        QString cmd = "echo 'Se requieren permisos de root para eliminar la categoría:'; su -c \"rm -f '" + escapedPath + "'\"";
        QStringList args;
        if (term == "gnome-terminal") {
            args << "--" << "/bin/sh" << "-c" << cmd;
        } else {
            args << "-e" << ("/bin/sh -c \"" + cmd.replace('"', "\\\"") + "\"");
        }
        QProcess::execute(term, args);
        return !QFile::exists(filePath);
    }

    return false;
}

void MainWindow::onDeleteClicked()
{
    QTreeWidgetItem *current = m_appTree->currentItem();
    if (!current) return;

    QString entryKey = current->data(0, Qt::UserRole).toString();

    // CASO A: Es una categoría
    if (entryKey.isEmpty()) {
        QString dirFilePath = current->data(0, Qt::UserRole + 2).toString();
        if (dirFilePath.isEmpty()) {
            QMessageBox::information(this, "Categoría del sistema",
                                     "Las categorías estándar de Freedesktop no se pueden eliminar.");
            return;
        }

        if (current->childCount() > 0) {
            QMessageBox::warning(this, "Categoría con aplicaciones",
                                 "La categoría '" + current->text(0) + "' aún contiene "
                                 + QString::number(current->childCount())
                                 + " aplicación(es).\nMueve o elimina primero sus lanzadores.");
            return;
        }

        auto reply = QMessageBox::question(
            this,
            "Eliminar categoría (requiere root)",
            "¿Estás seguro de que deseas eliminar la categoría '" + current->text(0) + "'?\n\n"
            "Se solicitará la contraseña de root para borrar:\n" + dirFilePath,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No
        );

        if (reply != QMessageBox::Yes) return;

        if (removeFileAsRoot(dirFilePath)) {
            QString xdgTag = current->data(0, Qt::UserRole + 1).toString();
            m_customCategoryNames.remove(xdgTag);
            delete current;
            clearEditorForm();
            m_deleteAct->setEnabled(false);
            statusBar()->showMessage("Categoría eliminada correctamente.", 5000);
        } else {
            QMessageBox::warning(this, "Operación cancelada o fallida",
                                 "No se eliminó la categoría (autenticación cancelada o permisos insuficientes).");
        }
        return;
    }

    // CASO B: Es un lanzador (.desktop) - No pide contraseña root
    if (!m_entries.contains(entryKey)) return;
    DesktopEntry &entry = m_entries[entryKey];
    QString currentPath = entry.filePath();

    auto reply = QMessageBox::question(
        this,
        "Eliminar lanzador",
        "¿Deseas eliminar el lanzador '" + entry.name() + "'?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (reply != QMessageBox::Yes) return;

    // Si somos root o el archivo pertenece al usuario (~/.local/share/applications)
    if (geteuid() == 0 || currentPath.startsWith(QDir::homePath())) {
        if (QFile::remove(currentPath)) {
            // Comprobar si existía una versión original del sistema que ahora vuelve a quedar activa
            QString systemFallback = "/usr/share/applications/" + entryKey;
            if (geteuid() != 0 && QFile::exists(systemFallback)) {
                DesktopEntry original(systemFallback);
                m_entries[entryKey] = original;
                current->setText(0, original.name());
                current->setIcon(0, QIcon::fromTheme(original.icon(), QIcon::fromTheme("application-x-executable")));
                onItemSelected(current, nullptr);
                statusBar()->showMessage("Personalización eliminada. Restaurado lanzador original del sistema.", 5000);
            } else {
                m_entries.remove(entryKey);
                delete current;
                clearEditorForm();
                m_deleteAct->setEnabled(false);
                statusBar()->showMessage("Lanzador eliminado: " + currentPath, 5000);
            }
        } else {
            QMessageBox::warning(this, "Error", "No se pudo eliminar el archivo:\n" + currentPath);
        }
    } else {
        // Usuario normal eliminando un lanzador del sistema (/usr/share/...):
        // Estándar XDG: crear copia local con Hidden=true y NoDisplay=true
        entry.setValue("Hidden", "true");
        entry.setNoDisplay(true);
        if (entry.save()) {
            m_entries.remove(entryKey);
            delete current;
            clearEditorForm();
            m_deleteAct->setEnabled(false);
            statusBar()->showMessage("Lanzador ocultado del menú del usuario (Hidden=true en ~/.local).", 5000);
        } else {
            QMessageBox::warning(this, "Error", "No se pudo ocultar el lanzador en el perfil local.");
        }
    }
}

QString MainWindow::stripXdgFieldCodes(const QString &execCmd) const
{
    QString cleaned = execCmd;
    cleaned.remove(QRegularExpression("\\s+%[fFuUdDnNickvm]"));
    return cleaned.trimmed();
}

QString MainWindow::detectTerminalEmulator() const
{
    const QStringList terminals = {
        "xfce4-terminal",
        "konsole",
        "lxterminal",
        "mate-terminal",
        "gnome-terminal",
        "alacritty",
        "xterm"
    };

    for (const QString &term : terminals) {
        if (!QStandardPaths::findExecutable(term).isEmpty()) {
            return term;
        }
    }
    return QString();
}

void MainWindow::onTestLauncherClicked()
{
    QString rawUserCmd = stripXdgFieldCodes(m_execEdit->text());
    if (rawUserCmd.isEmpty()) {
        QMessageBox::information(this, "Comando vacío", "Escribe un comando en el campo 'Comando (Exec)' antes de probar.");
        return;
    }

    RunMode mode = static_cast<RunMode>(m_runModeCombo->currentData().toInt());
    QString fullCmd = buildExecCommand(mode, rawUserCmd);
    QString workDir = m_pathEdit->text().trimmed();
    if (workDir.isEmpty() || !QDir(workDir).exists()) {
        workDir = QDir::homePath();
    }

    bool started = false;
    if (m_terminalCheck->isChecked()) {
        QString term = detectTerminalEmulator();
        if (term.isEmpty()) {
            QMessageBox::warning(this, "Terminal no encontrada", "No se encontró ningún emulador de terminal instalado en el sistema.");
            return;
        }
        QString shellWrapper = fullCmd + "; echo ''; read -p 'Presiona Enter para cerrar...'";
        QStringList args;
        if (term == "gnome-terminal") {
            args << "--" << "/bin/sh" << "-c" << shellWrapper;
        } else {
            args << "-e" << ("/bin/sh -c \"" + shellWrapper.replace('"', "\\\"") + "\"");
        }
        started = QProcess::startDetached(term, args, workDir);
    } else {
        started = QProcess::startDetached("/bin/sh", QStringList() << "-c" << fullCmd, workDir);
    }

    if (started) {
        statusBar()->showMessage("Probando comando: " + fullCmd, 4000);
    } else {
        QMessageBox::warning(this, "Error de ejecución", "No se pudo iniciar el comando:\n" + fullCmd);
    }
}

MainWindow::RunMode MainWindow::detectRunMode(const QString &rawExec, QString &cleanExec) const
{
    QString trimmed = rawExec.trimmed();

    const QString pkexecX11Prefix = "pkexec env DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY ";
    if (trimmed.startsWith(pkexecX11Prefix)) {
        cleanExec = trimmed.mid(pkexecX11Prefix.length()).trimmed();
        return RunAsPkexec;
    }
    if (trimmed.startsWith("pkexec ")) {
        cleanExec = trimmed.mid(7).trimmed();
        return RunAsPkexec;
    }
    if (trimmed.startsWith("sudo ")) {
        cleanExec = trimmed.mid(5).trimmed();
        return RunAsSudo;
    }
    if (trimmed.startsWith("su -c \"") && trimmed.endsWith('"') && trimmed.length() > 8) {
        cleanExec = trimmed.mid(7, trimmed.length() - 8).trimmed();
        return RunAsSu;
    }

    cleanExec = trimmed;
    return RunAsUser;
}

QString MainWindow::buildExecCommand(RunMode mode, const QString &cleanExec) const
{
    QString cmd = cleanExec.trimmed();
    if (cmd.isEmpty()) return QString();

    switch (mode) {
    case RunAsPkexec:
        return "pkexec env DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY " + cmd;
    case RunAsSudo:
        return "sudo " + cmd;
    case RunAsSu: {
        QString escaped = cmd;
        escaped.replace('"', "\\\"");
        return "su -c \"" + escaped + "\"";
    }
    case RunAsUser:
    default:
        return cmd;
    }
}

void MainWindow::onRunModeChanged(int index)
{
    RunMode mode = static_cast<RunMode>(m_runModeCombo->itemData(index).toInt());

    if (mode == RunAsSudo || mode == RunAsSu) {
        m_terminalCheck->setChecked(true);
    } else if (mode == RunAsPkexec) {
        m_terminalCheck->setChecked(false);
    }
}

void MainWindow::onItemSelected(QTreeWidgetItem *current, QTreeWidgetItem *)
{
    if (!current) {
        clearEditorForm();
        m_deleteAct->setEnabled(false);
        return;
    }

    QString key = current->data(0, Qt::UserRole).toString();

    // Si es una categoría (no tiene entryKey)
    if (key.isEmpty() || !m_entries.contains(key)) {
        clearEditorForm();
        QString dirFilePath = current->data(0, Qt::UserRole + 2).toString();
        m_deleteAct->setEnabled(!dirFilePath.isEmpty());
        if (!dirFilePath.isEmpty()) {
            statusBar()->showMessage("Categoría personalizada: " + dirFilePath);
        } else {
            statusBar()->showMessage("Categoría estándar del sistema: " + current->text(0));
        }
        return;
    }

    // Es un lanzador
    m_deleteAct->setEnabled(true);
    m_currentKey = key;
    const DesktopEntry &entry = m_entries[key];

    QString cleanExec;
    RunMode mode = detectRunMode(entry.exec(), cleanExec);

    m_nameEdit->setText(entry.name());
    m_commentEdit->setText(entry.comment());
    m_execEdit->setText(cleanExec);

    m_runModeCombo->blockSignals(true);
    int comboIdx = m_runModeCombo->findData(mode);
    m_runModeCombo->setCurrentIndex(comboIdx >= 0 ? comboIdx : 0);
    m_runModeCombo->blockSignals(false);

    m_iconEdit->setText(entry.icon());
    m_pathEdit->setText(entry.workingPath());
    m_categoriesEdit->setText(entry.categories().join(';'));
    m_terminalCheck->setChecked(entry.isTerminal());
    m_noDisplayCheck->setChecked(entry.isNoDisplay());

    onIconTextChanged(entry.icon());
    m_testButton->setEnabled(true);
    m_saveButton->setEnabled(true);
    statusBar()->showMessage("Archivo origen: " + entry.filePath());
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    for (int i = 0; i < m_appTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *catItem = m_appTree->topLevelItem(i);
        bool anyVisible = false;

        for (int j = 0; j < catItem->childCount(); ++j) {
            QTreeWidgetItem *appItem = catItem->child(j);
            bool match = text.isEmpty() || appItem->text(0).contains(text, Qt::CaseInsensitive);
            appItem->setHidden(!match);
            if (match) anyVisible = true;
        }

        catItem->setHidden(!anyVisible && !text.isEmpty());
        if (!text.isEmpty() && anyVisible) {
            catItem->setExpanded(true);
        }
    }
}

void MainWindow::onIconTextChanged(const QString &iconName)
{
    QIcon icon;
    if (QFileInfo::exists(iconName)) {
        icon = QIcon(iconName);
    } else {
        icon = QIcon::fromTheme(iconName, QIcon::fromTheme("application-x-executable"));
    }
    m_iconPreview->setPixmap(icon.pixmap(48, 48));
}

void MainWindow::onSaveClicked()
{
    if (m_currentKey.isEmpty() || !m_entries.contains(m_currentKey)) return;

    RunMode mode = static_cast<RunMode>(m_runModeCombo->currentData().toInt());
    QString finalExec = buildExecCommand(mode, m_execEdit->text());

    DesktopEntry &entry = m_entries[m_currentKey];
    entry.setName(m_nameEdit->text().trimmed());
    entry.setComment(m_commentEdit->text().trimmed());
    entry.setExec(finalExec);
    entry.setIcon(m_iconEdit->text().trimmed());
    entry.setWorkingPath(m_pathEdit->text().trimmed());
    entry.setCategories(m_categoriesEdit->text().split(';', Qt::SkipEmptyParts));
    entry.setTerminal(m_terminalCheck->isChecked());
    entry.setNoDisplay(m_noDisplayCheck->isChecked());

    if (entry.save()) {
        statusBar()->showMessage("Guardado en: " + entry.filePath(), 5000);
        if (QTreeWidgetItem *item = m_appTree->currentItem()) {
            item->setText(0, entry.name());
            item->setIcon(0, QIcon::fromTheme(entry.icon(), QIcon::fromTheme("application-x-executable")));
        }
    } else {
        QMessageBox::warning(this, "Error al guardar",
                             "No se pudo escribir en:\n" + entry.filePath());
    }
}

void MainWindow::onReloadClicked()
{
    loadApplications();
    statusBar()->showMessage("Lista de aplicaciones recargada.", 3000);
}
