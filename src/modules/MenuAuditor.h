#ifndef MENUAUDITOR_H
#define MENUAUDITOR_H

#include <QString>
#include <QList>
#include <QMap>
#include "DesktopEntry.h"

struct AuditEntryInfo {
    QString key;          // Nombre del archivo .desktop
    QString name;         // Nombre visible de la app
    QString filePath;     // Ruta completa al .desktop
    QString rawExec;      // Línea Exec original
    QString resolvedBin;  // Binario extraído o ruta canónica
};

struct DuplicateGroup {
    QString resolvedExecutable;
    QList<AuditEntryInfo> entries;
};

struct MenuAuditReport {
    QList<AuditEntryInfo> orphanedEntries;
    QList<DuplicateGroup> duplicateGroups;

    bool hasIssues() const {
        return !orphanedEntries.isEmpty() || !duplicateGroups.isEmpty();
    }
};

class MenuAuditor
{
public:
    static MenuAuditReport analyze(const QMap<QString, DesktopEntry> &entries);
    static QString extractExecutableToken(const QString &rawExec);
    static QString resolveExecutablePath(const QString &execToken);
};

#endif // MENUAUDITOR_H
