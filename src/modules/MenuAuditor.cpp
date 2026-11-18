#include "MenuAuditor.h"

#include <QFileInfo>
#include <QStandardPaths>
#include <QRegularExpression>

QString MenuAuditor::extractExecutableToken(const QString &rawExec)
{
    QString cmd = rawExec.trimmed();
    if (cmd.isEmpty()) return QString();

    // Quitar prefijo de pkexec con variables X11
    const QString pkexecX11 = "pkexec env DISPLAY=$DISPLAY XAUTHORITY=$XAUTHORITY ";
    if (cmd.startsWith(pkexecX11)) {
        cmd = cmd.mid(pkexecX11.length()).trimmed();
    } else if (cmd.startsWith("pkexec ")) {
        cmd = cmd.mid(7).trimmed();
    } else if (cmd.startsWith("sudo ")) {
        cmd = cmd.mid(5).trimmed();
    } else if (cmd.startsWith("su -c \"") && cmd.endsWith('"') && cmd.length() > 8) {
        cmd = cmd.mid(7, cmd.length() - 8).trimmed();
        cmd.replace("\\\"", "\"");
    }

    // Si empieza con "env VAR=VAL ...", avanzar hasta el comando real
    if (cmd.startsWith("env ")) {
        const QStringList parts = cmd.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        for (int i = 1; i < parts.size(); ++i) {
            if (!parts[i].contains('=')) {
                cmd = parts.mid(i).join(' ');
                break;
            }
        }
    }

    // Extraer el primer token respetando comillas si la ruta tiene espacios
    if (cmd.startsWith('"')) {
        int endQuote = cmd.indexOf('"', 1);
        if (endQuote > 1) {
            return cmd.mid(1, endQuote - 1);
        }
    } else if (cmd.startsWith('\'')) {
        int endQuote = cmd.indexOf('\'', 1);
        if (endQuote > 1) {
            return cmd.mid(1, endQuote - 1);
        }
    }

    int spaceIdx = cmd.indexOf(QRegularExpression("\\s"));
    return (spaceIdx > 0) ? cmd.left(spaceIdx) : cmd;
}

QString MenuAuditor::resolveExecutablePath(const QString &execToken)
{
    if (execToken.isEmpty()) return QString();

    // Si es una ruta explícita (/usr/bin/foo, /opt/app/bin, etc.)
    if (execToken.contains('/')) {
        QFileInfo fi(execToken);
        if (fi.exists() && fi.isFile() && fi.isExecutable()) {
            QString canonical = fi.canonicalFilePath();
            return canonical.isEmpty() ? fi.absoluteFilePath() : canonical;
        }
        return QString();
    }

    // Buscar en $PATH
    QString found = QStandardPaths::findExecutable(execToken);
    if (!found.isEmpty()) {
        QFileInfo fi(found);
        QString canonical = fi.canonicalFilePath();
        return canonical.isEmpty() ? found : canonical;
    }

    return QString();
}

MenuAuditReport MenuAuditor::analyze(const QMap<QString, DesktopEntry> &entries)
{
    MenuAuditReport report;
    QMap<QString, QList<AuditEntryInfo>> byBinary;

    for (auto it = entries.constBegin(); it != entries.constEnd(); ++it) {
        const DesktopEntry &entry = it.value();

        // Ignorar entradas ya ocultas explícitamente
        if (entry.isNoDisplay()) continue;

        QString rawExec = entry.exec();
        QString token = extractExecutableToken(rawExec);
        QString resolved = resolveExecutablePath(token);

        AuditEntryInfo info;
        info.key = it.key();
        info.name = entry.name();
        info.filePath = entry.filePath();
        info.rawExec = rawExec;
        info.resolvedBin = resolved.isEmpty() ? token : resolved;

        if (resolved.isEmpty()) {
            report.orphanedEntries.append(info);
        } else {
            byBinary[resolved].append(info);
        }
    }

    // Filtrar grupos con más de una entrada apuntando al mismo binario
    for (auto it = byBinary.constBegin(); it != byBinary.constEnd(); ++it) {
        if (it.value().size() > 1) {
            DuplicateGroup group;
            group.resolvedExecutable = it.key();
            group.entries = it.value();
            report.duplicateGroups.append(group);
        }
    }

    return report;
}
