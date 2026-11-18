#ifndef ICONPICKERDIALOG_H
#define ICONPICKERDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QPair>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QLabel;
class QTimer;

class IconPickerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IconPickerDialog(const QString &currentIcon, QWidget *parent = nullptr);
    QString selectedIcon() const { return m_selectedIcon; }

private slots:
    void onFilterChanged(const QString &text);
    void onItemClicked(QListWidgetItem *item);
    void onItemDoubleClicked(QListWidgetItem *item);
    void loadNextBatch();

private:
    void collectActiveThemeIcons();
    QString resolveActiveThemeName() const;

    QLineEdit *m_filterEdit = nullptr;
    QListWidget *m_iconList = nullptr;
    QLabel *m_statusLabel = nullptr;
    QTimer *m_batchTimer = nullptr;

    QString m_activeTheme;
    QString m_selectedIcon;
    QList<QPair<QString, QString>> m_pendingIcons; // <iconName, filePath>
    int m_loadedIndex = 0;
};

#endif // ICONPICKERDIALOG_H
