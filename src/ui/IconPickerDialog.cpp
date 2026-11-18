#include "IconPickerDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QDirIterator>
#include <QFileInfo>
#include <QDir>
#include <QSet>
#include <QTimer>
#include <QIcon>
#include <algorithm>

IconPickerDialog::IconPickerDialog(const QString &currentIcon, QWidget *parent)
    : QDialog(parent)
    , m_selectedIcon(currentIcon)
{
    m_activeTheme = resolveActiveThemeName();

    setWindowTitle("Iconos del sistema - Tema: " + m_activeTheme);
    resize(700, 500);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Barra superior de búsqueda
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText("Filtrar iconos del tema '" + m_activeTheme + "' (ej. firefox, terminal, folder)...");
    m_filterEdit->setClearButtonEnabled(true);
    mainLayout->addWidget(m_filterEdit);

    // Cuadrícula optimizada (solo renderiza los iconos visibles en pantalla)
    m_iconList = new QListWidget(this);
    m_iconList->setViewMode(QListView::IconMode);
    m_iconList->setLayoutMode(QListView::Batched);
    m_iconList->setBatchSize(120);
    m_iconList->setIconSize(QSize(36, 36));
    m_iconList->setGridSize(QSize(112, 78));
    m_iconList->setResizeMode(QListView::Adjust);
    m_iconList->setMovement(QListView::Static);
    m_iconList->setUniformItemSizes(true);
    m_iconList->setWordWrap(true);
    mainLayout->addWidget(m_iconList, 1);

    // Barra inferior: estado y botones Aceptar / Cancelar
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    m_statusLabel = new QLabel("Cargando iconos del tema " + m_activeTheme + "...", this);
    bottomLayout->addWidget(m_statusLabel, 1);

    QPushButton *cancelBtn = new QPushButton("Cancelar", this);
    QPushButton *okBtn = new QPushButton(QIcon::fromTheme("dialog-ok-apply"), "Aceptar", this);
    okBtn->setDefault(true);

    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(okBtn);
    mainLayout->addLayout(bottomLayout);

    connect(m_filterEdit, &QLineEdit::textChanged, this, &IconPickerDialog::onFilterChanged);
    connect(m_iconList, &QListWidget::itemClicked, this, &IconPickerDialog::onItemClicked);
    connect(m_iconList, &QListWidget::itemDoubleClicked, this, &IconPickerDialog::onItemDoubleClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);

    // Recolectar rutas del tema activo e insertarlas en lotes sin bloquear la UI
    collectActiveThemeIcons();

    m_batchTimer = new QTimer(this);
    m_batchTimer->setInterval(5);
    connect(m_batchTimer, &QTimer::timeout, this, &IconPickerDialog::loadNextBatch);
    m_batchTimer->start();
}

QString IconPickerDialog::resolveActiveThemeName() const
{
    QString theme = QIcon::themeName();
    if (!theme.isEmpty() && theme != "hicolor") {
        return theme;
    }

    // Fallback: leer tema configurado en XFCE si Qt no lo detectó automáticamente
    QString xfceXml = QDir::homePath() + "/.config/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml";
    QFile file(xfceXml);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(file.readAll());
        int idx = content.indexOf("name=\"IconThemeName\"");
        if (idx != -1) {
            int valStart = content.indexOf("value=\"", idx);
            if (valStart != -1) {
                valStart += 7;
                int valEnd = content.indexOf('"', valStart);
                if (valEnd > valStart) {
                    QString detected = content.mid(valStart, valEnd - valStart);
                    if (!detected.isEmpty()) {
                        QIcon::setThemeName(detected);
                        return detected;
                    }
                }
            }
        }
    }

    return theme.isEmpty() ? "hicolor" : theme;
}

void IconPickerDialog::collectActiveThemeIcons()
{
    QSet<QString> seenNames;
    const QStringList themeRoots = {
        QDir::homePath() + "/.local/share/icons/" + m_activeTheme,
        QDir::homePath() + "/.icons/" + m_activeTheme,
        "/usr/local/share/icons/" + m_activeTheme,
        "/usr/share/icons/" + m_activeTheme
    };

    const QStringList filters = {"*.png", "*.svg", "*.xpm"};

    for (const QString &themeDir : themeRoots) {
        if (!QFileInfo::exists(themeDir)) continue;

        QDirIterator it(themeDir, filters, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();

            // Ignorar cursores y tamaños diminutos redundantes
            if (filePath.contains("/cursors/") || filePath.contains("/16x16/") || filePath.contains("/22x22/") || filePath.contains("/24x24/")) {
                continue;
            }

            QFileInfo fi(filePath);
            QString iconName = fi.completeBaseName();

            // Evitar variantes simbólicas duplicadas si ya existe el icono estándar
            if (seenNames.contains(iconName)) continue;
            seenNames.insert(iconName);

            m_pendingIcons.append(qMakePair(iconName, filePath));
        }
    }

    std::sort(m_pendingIcons.begin(), m_pendingIcons.end(),
              [](const QPair<QString, QString> &a, const QPair<QString, QString> &b) {
                  return a.first.compare(b.first, Qt::CaseInsensitive) < 0;
              });
}

void IconPickerDialog::loadNextBatch()
{
    const int batchCount = 150;
    int end = qMin(m_loadedIndex + batchCount, m_pendingIcons.size());
    const QString filterText = m_filterEdit->text();

    m_iconList->setUpdatesEnabled(false);
    for (int i = m_loadedIndex; i < end; ++i) {
        const QString &iconName = m_pendingIcons[i].first;
        const QString &filePath = m_pendingIcons[i].second;

        // QIcon::fromTheme carga de forma diferida solo los iconos visibles en pantalla
        QIcon icon = QIcon::fromTheme(iconName);
        if (icon.isNull()) {
            icon = QIcon(filePath);
        }

        auto *item = new QListWidgetItem(icon, iconName, m_iconList);
        item->setData(Qt::UserRole, iconName);
        item->setToolTip(iconName + "\n(" + filePath + ")");

        if (!filterText.isEmpty() && !iconName.contains(filterText, Qt::CaseInsensitive)) {
            item->setHidden(true);
        }
    }
    m_iconList->setUpdatesEnabled(true);

    m_loadedIndex = end;

    if (m_loadedIndex >= m_pendingIcons.size()) {
        m_batchTimer->stop();
        m_statusLabel->setText(QString("Tema '%1': %2 iconos disponibles.").arg(m_activeTheme).arg(m_iconList->count()));
    } else {
        m_statusLabel->setText(QString("Cargando iconos (%1 / %2)...").arg(m_loadedIndex).arg(m_pendingIcons.size()));
    }
}

void IconPickerDialog::onFilterChanged(const QString &text)
{
    for (int i = 0; i < m_iconList->count(); ++i) {
        QListWidgetItem *item = m_iconList->item(i);
        bool match = text.isEmpty() || item->text().contains(text, Qt::CaseInsensitive);
        item->setHidden(!match);
    }
}

void IconPickerDialog::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    m_selectedIcon = item->data(Qt::UserRole).toString();
    m_statusLabel->setText("Seleccionado: " + m_selectedIcon);
}

void IconPickerDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;
    m_selectedIcon = item->data(Qt::UserRole).toString();
    accept();
}
