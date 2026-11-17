#include "DesktopEntry.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QStandardPaths>
#include <QLocale>
#include <unistd.h>

DesktopEntry::DesktopEntry()
{
    m_rawLines << "[Desktop Entry]"
               << "Type=Application"
               << "Version=1.0";
    m_entries["Type"] = "Application";
    m_entries["Version"] = "1.0";
}

DesktopEntry::DesktopEntry(const QString &filePath)
{
    load(filePath);
}

bool DesktopEntry::load(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_valid = false;
        return false;
    }

    m_filePath = filePath;
    m_rawLines.clear();
    m_entries.clear();

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    bool inDesktopEntrySection = false;

    while (!in.atEnd()) {
        QString line = in.readLine();
        m_rawLines.append(line);

        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }

        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            inDesktopEntrySection = (trimmed == "[Desktop Entry]");
            continue;
        }

        if (inDesktopEntrySection) {
            int eqIndex = line.indexOf('=');
            if (eqIndex > 0) {
                QString key = line.left(eqIndex).trimmed();
                QString val = line.mid(eqIndex + 1).trimmed();
                m_entries.insert(key, val);
            }
        }
    }

    m_valid = m_entries.contains("Name") || !m_rawLines.isEmpty();
    return m_valid;
}

bool DesktopEntry::save(const QString &customPath)
{
    QString targetPath = customPath.isEmpty() ? m_filePath : customPath;

    // Si no somos root y el archivo está fuera de HOME (ej. /usr/share/applications),
    // redirigimos el guardado al directorio local XDG del usuario.
    if (geteuid() != 0 && !targetPath.startsWith(QDir::homePath())) {
        QString localApps = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
        QDir().mkpath(localApps);
        targetPath = localApps + "/" + fileName();
    }

    QFile file(targetPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    out.setCodec("UTF-8");
#endif

    QMap<QString, QString> pending = m_entries;
    bool inDesktopEntrySection = false;
    int insertIndex = -1;

    // Actualizamos las líneas existentes preservando comentarios y estructura
    for (int i = 0; i < m_rawLines.size(); ++i) {
        QString trimmed = m_rawLines[i].trimmed();

        if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
            if (inDesktopEntrySection) {
                insertIndex = i;
            }
            inDesktopEntrySection = (trimmed == "[Desktop Entry]");
            continue;
        }

        if (inDesktopEntrySection && !trimmed.isEmpty() && !trimmed.startsWith('#')) {
            int eqIndex = m_rawLines[i].indexOf('=');
            if (eqIndex > 0) {
                QString key = m_rawLines[i].left(eqIndex).trimmed();
                if (pending.contains(key)) {
                    m_rawLines[i] = key + "=" + pending.take(key);
                }
            }
        }
    }

    // Insertar claves nuevas al final de la sección [Desktop Entry]
    if (!pending.isEmpty()) {
        if (insertIndex == -1) {
            insertIndex = m_rawLines.size();
        }
        for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
            m_rawLines.insert(insertIndex++, it.key() + "=" + it.value());
        }
    }

    for (const QString &line : qAsConst(m_rawLines)) {
        out << line << "\n";
    }

    m_filePath = targetPath;
    m_valid = true;
    return true;
}

QString DesktopEntry::fileName() const
{
    return QFileInfo(m_filePath).fileName();
}

QString DesktopEntry::name() const
{
    QString lang = QLocale::system().name().section('_', 0, 0);
    if (m_entries.contains("Name[" + lang + "]")) {
        return m_entries.value("Name[" + lang + "]");
    }
    return m_entries.value("Name");
}

QString DesktopEntry::comment() const
{
    QString lang = QLocale::system().name().section('_', 0, 0);
    if (m_entries.contains("Comment[" + lang + "]")) {
        return m_entries.value("Comment[" + lang + "]");
    }
    return m_entries.value("Comment");
}

QString DesktopEntry::exec() const { return m_entries.value("Exec"); }
QString DesktopEntry::icon() const { return m_entries.value("Icon"); }
QString DesktopEntry::workingPath() const { return m_entries.value("Path"); }

QStringList DesktopEntry::categories() const
{
    return m_entries.value("Categories").split(';', Qt::SkipEmptyParts);
}

bool DesktopEntry::isTerminal() const
{
    return m_entries.value("Terminal").compare("true", Qt::CaseInsensitive) == 0;
}

bool DesktopEntry::isNoDisplay() const
{
    return m_entries.value("NoDisplay").compare("true", Qt::CaseInsensitive) == 0;
}

void DesktopEntry::setName(const QString &name)
{
    QString lang = QLocale::system().name().section('_', 0, 0);
    if (m_entries.contains("Name[" + lang + "]")) {
        m_entries["Name[" + lang + "]"] = name;
    }
    m_entries["Name"] = name;
}

void DesktopEntry::setComment(const QString &comment)
{
    QString lang = QLocale::system().name().section('_', 0, 0);
    if (m_entries.contains("Comment[" + lang + "]")) {
        m_entries["Comment[" + lang + "]"] = comment;
    }
    m_entries["Comment"] = comment;
}

void DesktopEntry::setExec(const QString &exec) { m_entries["Exec"] = exec; }
void DesktopEntry::setIcon(const QString &icon) { m_entries["Icon"] = icon; }
void DesktopEntry::setWorkingPath(const QString &path) { m_entries["Path"] = path; }

void DesktopEntry::setCategories(const QStringList &categories)
{
    QString joined = categories.join(';');
    if (!joined.isEmpty() && !joined.endsWith(';')) {
        joined += ';';
    }
    m_entries["Categories"] = joined;
}

void DesktopEntry::setTerminal(bool terminal)
{
    m_entries["Terminal"] = terminal ? "true" : "false";
}

void DesktopEntry::setNoDisplay(bool noDisplay)
{
    m_entries["NoDisplay"] = noDisplay ? "true" : "false";
}

QString DesktopEntry::value(const QString &key, const QString &defaultValue) const
{
    return m_entries.value(key, defaultValue);
}

void DesktopEntry::setValue(const QString &key, const QString &value)
{
    m_entries[key] = value;
}
