#ifndef DESKTOPENTRY_H
#define DESKTOPENTRY_H

#include <QString>
#include <QStringList>
#include <QMap>

class DesktopEntry
{
public:
    DesktopEntry();
    explicit DesktopEntry(const QString &filePath);

    bool load(const QString &filePath);
    bool save(const QString &customPath = QString());

    bool isValid() const { return m_valid; }
    QString filePath() const { return m_filePath; }
    QString fileName() const;

    // Getters principales
    QString name() const;
    QString comment() const;
    QString exec() const;
    QString icon() const;
    QString workingPath() const;
    QStringList categories() const;
    bool isTerminal() const;
    bool isNoDisplay() const;

    // Setters principales
    void setName(const QString &name);
    void setComment(const QString &comment);
    void setExec(const QString &exec);
    void setIcon(const QString &icon);
    void setWorkingPath(const QString &path);
    void setCategories(const QStringList &categories);
    void setTerminal(bool terminal);
    void setNoDisplay(bool noDisplay);

    // Acceso genérico a claves dentro de [Desktop Entry]
    QString value(const QString &key, const QString &defaultValue = QString()) const;
    void setValue(const QString &key, const QString &value);

private:
    QString m_filePath;
    QStringList m_rawLines;
    QMap<QString, QString> m_entries;
    bool m_valid = false;
};

#endif // DESKTOPENTRY_H
