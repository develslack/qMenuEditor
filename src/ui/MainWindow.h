#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include "DesktopEntry.h"

class QAction;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QComboBox;
class QCheckBox;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum RunMode {
        RunAsUser = 0,
        RunAsPkexec,
        RunAsSudo,
        RunAsSu
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onItemSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void onSearchTextChanged(const QString &text);
    void onIconTextChanged(const QString &iconName);
    void onIconButtonClicked();
    void onBrowseSystemIcons();
    void onBrowseIconFile();
    void onRunModeChanged(int index);
    void onNewLauncherClicked();
    void onNewCategoryClicked();
    void onDeleteClicked();
    void onAuditMenuClicked();
    void onTestLauncherClicked();
    void onSaveClicked();
    void onReloadClicked();
    void onAboutClicked();

private:
    void setupUi();
    void loadApplications();
    void runMenuAudit(bool silentIfClean);
    bool removeOrHideEntry(const QString &entryKey);
    void loadCustomCategories(QMap<QString, QTreeWidgetItem *> &categoryNodes);
    QString mapMainCategory(const QStringList &categories, const QMap<QString, QTreeWidgetItem *> &categoryNodes) const;
    QString categoryToXdgTag(const QString &displayName) const;
    void clearEditorForm();
    bool removeFileAsRoot(const QString &filePath);

    // Utilidades para extraer, limpiar y construir el comando según privilegios
    RunMode detectRunMode(const QString &rawExec, QString &cleanExec) const;
    QString buildExecCommand(RunMode mode, const QString &cleanExec) const;
    QString stripXdgFieldCodes(const QString &execCmd) const;
    QString detectTerminalEmulator() const;

    // Acciones de barra de herramientas
    QAction *m_deleteAct = nullptr;

    // Widgets de navegación
    QLineEdit *m_searchEdit = nullptr;
    QTreeWidget *m_appTree = nullptr;

    // Widgets del editor
    QPushButton *m_iconButton = nullptr;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_commentEdit = nullptr;
    QLineEdit *m_execEdit = nullptr;
    QComboBox *m_runModeCombo = nullptr;
    QLineEdit *m_iconEdit = nullptr;
    QLineEdit *m_pathEdit = nullptr;
    QLineEdit *m_categoriesEdit = nullptr;
    QCheckBox *m_terminalCheck = nullptr;
    QCheckBox *m_noDisplayCheck = nullptr;
    QPushButton *m_testButton = nullptr;
    QPushButton *m_saveButton = nullptr;

    // Estado interno
    QMap<QString, DesktopEntry> m_entries;
    QMap<QString, QString> m_customCategoryNames;
    QString m_currentKey;
};

#endif // MAINWINDOW_H
