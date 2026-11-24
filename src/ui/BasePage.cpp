#include "BasePage.h"
#include "AppContext.h"
#include "Icons.h"
#include "ThemeManager.h"
#include "ui_BasePage.h"

#include <QAction>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <utility>

namespace phy104 {

BasePage::BasePage(const QString& title, const QString& subtitle, AppContext* context, QWidget* parent, bool showHeader)
    : QWidget(parent), m_ctx(context) {
    Ui::BasePage form;
    form.setupUi(this);

    setObjectName("pageArea");
    form.pageInner->setObjectName("pageInner");
    m_root = form.rootLayout;
    m_content = form.contentLayout;
    m_columnsLayout = form.threeColumnsLayout;
    m_firstColumnLayout = form.firstColumnLayout;
    m_centerColumnLayout = form.centerColumnLayout;
    m_lastColumnLayout = form.lastColumnLayout;
    m_columnsHost = form.threeColumnsHost;
    m_firstColumn = form.firstColumn;
    m_lastColumn = form.lastColumn;
    m_saveButton = form.outline;
    m_exportButton = showHeader ? form.exportMenu : nullptr;
    if (!showHeader) delete form.exportMenu;

    form.pageTitle->setObjectName("pageTitle");
    form.pageSubtitle->setObjectName("pageSubtitle");
    form.pageTitle->setText(title);
    form.pageSubtitle->setText(subtitle);
    form.pageSubtitle->setVisible(!subtitle.isEmpty());
    form.pageHeader->setVisible(showHeader);
    if (!showHeader) {
        m_root->setContentsMargins(0, 0, 0, 0);
        m_root->setSpacing(0);
    }

    m_saveButton->setCursor(Qt::PointingHandCursor);
    if (m_exportButton) {
        m_exportButton->setCursor(Qt::PointingHandCursor);
        auto* exportMenu = new QMenu(m_exportButton);
        for (const auto& entry : {std::pair<const char*, int>{"PDF report…", 0}, {"Excel (.xls)…", 1}, {"CSV (.csv)…", 2}}) {
            QAction* action = exportMenu->addAction(QString::fromUtf8(entry.first));
            const int format = entry.second;
            connect(action, &QAction::triggered, this, [this, format] { emit exportFormatRequested(format); });
        }
        m_exportButton->setMenu(exportMenu);
        const auto refreshExportIcon = [this] {
            m_exportButton->setIcon(Icons::pixmap("download", ThemeManager::instance().theme().amber, 18));
        };
        refreshExportIcon();
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshExportIcon);
    }

    connect(m_saveButton, &QPushButton::clicked, this, [this] {
        if (m_ctx->saveSession()) emit m_ctx->message(QString(" ") + m_ctx->sessionFile);
    });

}

void BasePage::setCompactLayout() {
    if (!m_root) return;
    m_root->setContentsMargins(12, 10, 12, 8);
    m_root->setSpacing(8);
    m_content->setSpacing(8);
}

void BasePage::useSaveExportHeader() {
    // Retained for existing page constructors: every BasePage now has this same header by default.
    // Keep the header helper idempotent in case a derived page calls this more than once.
    if (!m_saveButton) return;
    m_saveButton->setText("Save");
    m_saveButton->setMinimumWidth(130);
}

void BasePage::setStatusInfo(const StatusInfo& info, bool paused) {
    m_status = info;
    m_paused = paused;
    emit stateChanged();
}

BasePage::Columns BasePage::addThreeColumns(int leftWidth, int rightWidth) {
    Columns c;
    const bool rtl = (layoutDirection() == Qt::RightToLeft);
    const int firstWidth = rtl ? rightWidth : leftWidth;
    const int lastWidth = rtl ? leftWidth : rightWidth;
    auto setColumnWidth = [](QWidget* column, int width) {
        if (width > 0) column->setFixedWidth(width);
        else {
            column->setMinimumWidth(0);
            column->setMaximumWidth(QWIDGETSIZE_MAX);
        }
    };

    setColumnWidth(m_firstColumn, firstWidth);
    setColumnWidth(m_lastColumn, lastWidth);
    m_columnsLayout->setStretch(0, firstWidth > 0 ? 0 : 1);
    m_columnsLayout->setStretch(1, 1);
    m_columnsLayout->setStretch(2, lastWidth > 0 ? 0 : 1);

    m_columnsHost->show();
    if (rtl) {
        c.left = m_lastColumnLayout;
        c.right = m_firstColumnLayout;
    } else {
        c.left = m_firstColumnLayout;
        c.right = m_lastColumnLayout;
    }
    c.center = m_centerColumnLayout;
    return c;
}

} // namespace phy104
