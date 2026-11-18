#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QString>

class QTabWidget;

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);

private slots:
    void onShowGplClicked();
    void onShowLgplClicked();
    void onShowBsdClicked();

private:
    QWidget *createSystemTab();
    QWidget *createAboutTab();
    QWidget *createCreditsTab();
    QWidget *createDistroTab(QString &outDistroTabTitle);
    QWidget *createCopyrightTab();

    void showLicenseDialog(const QString &title, const QString &fileName);
    QString readLicenseFile(const QString &fileName) const;

    // Consultas dinámicas del sistema
    QString detectCpuModel() const;
    QString detectTotalMemory() const;
    QString detectGpuModel() const;
    QString detectDesktopEnvironment() const;
    QMap<QString, QString> readOsRelease() const;
};

#endif // ABOUTDIALOG_H
