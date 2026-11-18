#ifndef MENUAUDITDIALOG_H
#define MENUAUDITDIALOG_H

#include <QDialog>
#include <QStringList>
#include "MenuAuditor.h"

class QTreeWidget;

class MenuAuditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MenuAuditDialog(const MenuAuditReport &report, QWidget *parent = nullptr);
    QStringList selectedKeysToClean() const;

private:
    QTreeWidget *m_orphansTree = nullptr;
    QTreeWidget *m_duplicatesTree = nullptr;
};

#endif // MENUAUDITDIALOG_H
