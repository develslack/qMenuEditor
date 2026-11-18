#include "MenuAuditDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QIcon>

MenuAuditDialog::MenuAuditDialog(const MenuAuditReport &report, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Auditoría del Menú - Entradas huérfanas y duplicadas");
    resize(780, 480);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *headerLabel = new QLabel(
        QString("Se detectaron <b>%1 entrada(s) huérfana(s)</b> y <b>%2 grupo(s) de aplicaciones repetidas</b>.<br>"
                "Marca las entradas que deseas eliminar u ocultar del menú:")
            .arg(report.orphanedEntries.size())
            .arg(report.duplicateGroups.size()),
        this
    );
    headerLabel->setWordWrap(true);
    mainLayout->addWidget(headerLabel);

    QTabWidget *tabs = new QTabWidget(this);

    // Pestaña 1: Entradas huérfanas
    m_orphansTree = new QTreeWidget(tabs);
    m_orphansTree->setHeaderLabels({"Aplicación", "Comando faltante", "Archivo .desktop"});
    m_orphansTree->setColumnWidth(0, 210);
    m_orphansTree->setColumnWidth(1, 220);

    for (const AuditEntryInfo &info : report.orphanedEntries) {
        auto *item = new QTreeWidgetItem(m_orphansTree, {info.name, info.resolvedBin, info.filePath});
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Checked);
        item->setData(0, Qt::UserRole, info.key);
        item->setIcon(0, QIcon::fromTheme("dialog-warning"));
    }
    tabs->addTab(m_orphansTree, QString("Huérfanas (%1)").arg(report.orphanedEntries.size()));

    // Pestaña 2: Aplicaciones repetidas
    m_duplicatesTree = new QTreeWidget(tabs);
    m_duplicatesTree->setHeaderLabels({"Ejecutable / Aplicación", "Archivo .desktop"});
    m_duplicatesTree->setColumnWidth(0, 340);

    for (const DuplicateGroup &group : report.duplicateGroups) {
        auto *groupItem = new QTreeWidgetItem(m_duplicatesTree, {"Binario: " + group.resolvedExecutable, ""});
        groupItem->setIcon(0, QIcon::fromTheme("application-x-executable"));
        QFont boldFont = groupItem->font(0);
        boldFont.setBold(true);
        groupItem->setFont(0, boldFont);

        for (int i = 0; i < group.entries.size(); ++i) {
            const AuditEntryInfo &info = group.entries[i];
            auto *entryItem = new QTreeWidgetItem(groupItem, {info.name, info.filePath});
            entryItem->setFlags(entryItem->flags() | Qt::ItemIsUserCheckable);
            // Dejamos desmarcada la primera entrada (principal) y marcamos las copias repetidas
            entryItem->setCheckState(0, (i == 0) ? Qt::Unchecked : Qt::Checked);
            entryItem->setData(0, Qt::UserRole, info.key);
        }
        groupItem->setExpanded(true);
    }
    tabs->addTab(m_duplicatesTree, QString("Repetidas (%1)").arg(report.duplicateGroups.size()));

    mainLayout->addWidget(tabs, 1);

    // Botones inferiores
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton *ignoreBtn = new QPushButton("Ignorar por ahora", this);
    QPushButton *cleanBtn = new QPushButton(QIcon::fromTheme("edit-clear"), "Limpiar seleccionados", this);
    cleanBtn->setDefault(true);

    btnLayout->addWidget(ignoreBtn);
    btnLayout->addWidget(cleanBtn);
    mainLayout->addLayout(btnLayout);

    connect(ignoreBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(cleanBtn, &QPushButton::clicked, this, &QDialog::accept);
}

QStringList MenuAuditDialog::selectedKeysToClean() const
{
    QStringList keys;

    for (int i = 0; i < m_orphansTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_orphansTree->topLevelItem(i);
        if (item->checkState(0) == Qt::Checked) {
            keys.append(item->data(0, Qt::UserRole).toString());
        }
    }

    for (int i = 0; i < m_duplicatesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *groupItem = m_duplicatesTree->topLevelItem(i);
        for (int j = 0; j < groupItem->childCount(); ++j) {
            QTreeWidgetItem *entryItem = groupItem->child(j);
            if (entryItem->checkState(0) == Qt::Checked) {
                keys.append(entryItem->data(0, Qt::UserRole).toString());
            }
        }
    }

    return keys;
}
