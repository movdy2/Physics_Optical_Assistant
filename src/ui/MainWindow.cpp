#include "MainWindow.h"
#include "AppContext.h"
#include "BasePage.h"
#include "ExportPage.h"
#include "Icons.h"
#include "LanguageManager.h"
#include "ThemeManager.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QButtonGroup>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>

namespace phy104 {
namespace {

/// Sidebar item. The background / border come from the style sheet; icon and label are painted here
/// so they are always aligned and vertically centred (also for the two-line "Wave Interference").
class NavButton : public QPushButton {
public:
    NavButton(const QString& title, const QString& icon, QWidget* parent = nullptr) : QPushButton(parent), m_title(title), m_icon(icon) {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::NoFocus);
        setAccessibleName(title);
        setToolTip(QString(title).replace('\n', ' '));
        refreshPixmaps();
        connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] { refreshPixmaps(); update(); });
    }
    QString title() const { return m_title; }

protected:
    void paintEvent(QPaintEvent* e) override {
        QPushButton::paintEvent(e);                                   // style-sheet background and border
        const Theme& t = ThemeManager::instance().theme();
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        const int iconSize = 18, left = 14;
        const bool rtl = layoutDirection() == Qt::RightToLeft;
        const int iconX = rtl ? width() - left - iconSize : left;
        p.drawPixmap(QRect(iconX, (height() - iconSize) / 2, iconSize, iconSize), isChecked() ? m_on : m_off);
        QFont f(ThemeManager::uiFont());
        f.setFamilies(ThemeManager::uiFontFamilies());
        f.setPixelSize(12);
        f.setWeight(isChecked() ? QFont::Medium : QFont::Normal);
        p.setFont(f);
        p.setPen(isChecked() ? t.amber : t.text);
        const int textLeft = left + iconSize + 12;
        const QRect textRect(rtl ? 8 : textLeft, 0, width() - textLeft - 8, height());
        const Qt::Alignment align = rtl ? Qt::AlignVCenter | Qt::AlignRight : Qt::AlignVCenter | Qt::AlignLeft;
        p.drawText(textRect, align | Qt::TextWordWrap, LanguageManager::instance().text(m_title));
    }

private:
    void refreshPixmaps() {
        const Theme& t = ThemeManager::instance().theme();
        m_off = Icons::pixmap(m_icon, t.text, 18);
        m_on = Icons::pixmap(m_icon, t.amber, 18);
    }
    QString m_title, m_icon;
    QPixmap m_off, m_on;
};

} // namespace

MainWindow::MainWindow(AppContext* ctx, QWidget* parent)
    : QMainWindow(parent), m_ctx(ctx) {
    setWindowTitle(QString());
    const auto applyWindowIcon = [this] {
        const Theme& t = ThemeManager::instance().theme();
        setWindowIcon(Icons::get("scan-line", t.amber, t.amber, 64));     // taskbar / alt-tab icon
    };
    applyWindowIcon();
    resize(1280, 800);
    setMinimumSize(1080, 700);

    auto* central = new QWidget;
    central->setObjectName("central");
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ------------------------------------------------------------------ sidebar
    auto* sidebar = new QFrame;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(224);
    auto* sl = new QVBoxLayout(sidebar);
    sl->setContentsMargins(0, 0, 0, 0);
    sl->setSpacing(0);

    // Keep the small app mark, but remove the sidebar/window title block.
    auto* brand = new QWidget;
    brand->setObjectName("sidebarMark");
    auto* bl = new QHBoxLayout(brand);
    bl->setContentsMargins(20, 16, 20, 10);
    auto* logo = new QLabel;
    logo->setPixmap(Icons::pixmap("scan-line", ThemeManager::instance().theme().amber, 24));
    logo->setAlignment(Qt::AlignVCenter);
    bl->addWidget(logo);
    bl->addStretch();
    sl->addWidget(brand);

    auto* navigation = new QWidget;
    navigation->setObjectName("navigationContents");
    auto* navigationLayout = new QVBoxLayout(navigation);
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(0);
    m_modulesHeader = new QLabel("EXPERIMENTS");
    m_modulesHeader->setObjectName("navSection");
    navigationLayout->addWidget(m_modulesHeader);
    auto* navHolder = new QWidget;
    m_modules = new QVBoxLayout(navHolder);
    m_modules->setContentsMargins(12, 0, 12, 0);
    m_modules->setSpacing(4);
    navigationLayout->addWidget(navHolder);

    m_toolsHeader = new QLabel("TOOLS");
    m_toolsHeader->setObjectName("navSection");
    navigationLayout->addWidget(m_toolsHeader);
    auto* toolsHolder = new QWidget;
    m_tools = new QVBoxLayout(toolsHolder);
    m_tools->setContentsMargins(12, 0, 12, 0);
    m_tools->setSpacing(2);
    navigationLayout->addWidget(toolsHolder);
    navigationLayout->addStretch(1);

    auto* navigationScroll = new QScrollArea;
    navigationScroll->setObjectName("navigationScroll");
    navigationScroll->setFrameShape(QFrame::NoFrame);
    navigationScroll->setWidgetResizable(true);
    navigationScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navigationScroll->setWidget(navigation);
    sl->addWidget(navigationScroll, 1);

    auto* activity = new QWidget;
    activity->setObjectName("activityPanel");
    auto* activityLayout = new QVBoxLayout(activity);
    activityLayout->setContentsMargins(14, 8, 14, 10);
    activityLayout->setSpacing(4);
    auto* activityTitle = new QLabel("RECENT ACTIVITY");
    activityTitle->setObjectName("activityTitle");
    m_activityLog = new QListWidget;
    m_activityLog->setObjectName("activityLog");
    m_activityLog->setSelectionMode(QAbstractItemView::NoSelection);
    m_activityLog->setFocusPolicy(Qt::NoFocus);
    m_activityLog->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_activityLog->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_activityLog->setTextElideMode(Qt::ElideRight);
    m_activityLog->setWordWrap(false);
    m_activityLog->setMinimumHeight(56);
    m_activityLog->setMaximumHeight(74);
    activityLayout->addWidget(activityTitle);
    activityLayout->addWidget(m_activityLog);
    sl->addWidget(activity);
    root->addWidget(sidebar);

    // ------------------------------------------------------------------ pages
    auto* right = new QVBoxLayout;
    right->setContentsMargins(0, 0, 0, 0);
    right->setSpacing(0);
    m_stack = new QStackedWidget;
    m_stack->setObjectName("pageStack");
    right->addWidget(m_stack, 1);
    root->addLayout(right, 1);
    setCentralWidget(central);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    // ------------------------------------------------------------------ status bar
    auto plain = [](const QString& s) { auto* l = new QLabel(s); l->setObjectName("statusText"); return l; };
    m_statusLeft = plain("Ready");
    m_statusMid = plain("Double precision");
    m_statusRight = plain("Angles measured from normal");
    statusBar()->setSizeGripEnabled(false);
    statusBar()->addWidget(m_statusLeft);
    statusBar()->addWidget(plain("|"));
    statusBar()->addWidget(m_statusMid);
    statusBar()->addWidget(plain("|"));
    statusBar()->addWidget(m_statusRight);
    m_languageButton = new QPushButton;
    m_languageButton->setObjectName("flat");
    m_languageButton->setFocusPolicy(Qt::NoFocus);
    m_languageButton->setProperty("i18nSkip", true);
    m_themeButton = new QPushButton("☾ Dark");
    m_themeButton->setObjectName("flat");
    m_themeButton->setFocusPolicy(Qt::NoFocus);
    refreshLanguageButton();
    statusBar()->addPermanentWidget(m_languageButton);
    statusBar()->addPermanentWidget(m_themeButton);

    addActivity("Application ready");
    m_lastRunCount = ctx->log.savedCalculations();
    const auto recentRuns = ctx->log.recent(2);
    for (auto it = recentRuns.rbegin(); it != recentRuns.rend(); ++it)
        addActivity("Calculated", QString("%1 · %2").arg(QString::fromStdString(SessionLog::runLabel(it->id)),
                                                         QString::fromStdString(it->experiment)));
    if (ctx->log.hasExport()) addActivity("Exported", QFileInfo(QString::fromStdString(ctx->log.lastExport().path)).fileName());
    connect(ctx, &AppContext::runsChanged, this, [this] {
        const int count = m_ctx->log.savedCalculations();
        if (count > m_lastRunCount && !m_ctx->log.runs().empty()) {
            const RunRecord& run = m_ctx->log.runs().back();
            addActivity("Calculated", QString("%1 · %2").arg(QString::fromStdString(SessionLog::runLabel(run.id)),
                                                               QString::fromStdString(run.experiment)));
        } else if (count < m_lastRunCount) {
            addActivity("Removed saved calculation");
        }
        m_lastRunCount = count;
    });
    connect(ctx, &AppContext::exportRecorded, this, [this] {
        addActivity("Exported", QFileInfo(QString::fromStdString(m_ctx->log.lastExport().path)).fileName());
    });
    connect(ctx, &AppContext::notesAppended, this, [this] { addActivity("Sent result to Notes"); });

    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
    m_messageTimer->setInterval(4500);
    connect(m_messageTimer, &QTimer::timeout, this, [this] { m_transient.clear(); refreshStatus(); });
    connect(ctx, &AppContext::message, this, &MainWindow::showMessage);
    connect(m_languageButton, &QPushButton::clicked, this, [this] {
        auto& language = LanguageManager::instance();
        language.setLanguage(language.isArabic() ? UiLanguage::English : UiLanguage::Arabic);
        ThemeManager::instance().apply(*qApp, ThemeManager::instance().isDark());
        ThemeManager::instance().setLayoutDirectionForLanguage(language.isArabic());
        refreshStatus();
        language.apply(this);
        refreshActivityLog();
        refreshLanguageButton();
    });
    connect(m_themeButton, &QPushButton::clicked, this, [this] { emit themeToggled(!ThemeManager::instance().isDark()); });
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this, applyWindowIcon] {
        refreshIcons();
        applyWindowIcon();
        m_themeButton->setText(ThemeManager::instance().isDark() ? "☾ Dark" : "☀ Light");
    });
    connect(m_stack, &QStackedWidget::currentChanged, this, [this] { refreshStatus(); });
    auto* fullScreen = new QShortcut(QKeySequence(Qt::Key_F11), this);            // F11: toggle full screen
    connect(fullScreen, &QShortcut::activated, this, [this] { isFullScreen() ? showMaximized() : showFullScreen(); });
    installShortcuts();
}

void MainWindow::installShortcuts() {
    const auto add = [this](const QKeySequence& key, auto slot) {
        auto* shortcut = new QShortcut(key, this);
        connect(shortcut, &QShortcut::activated, this, slot);
        m_shortcutKeys << key.toString(QKeySequence::NativeText);
    };
    add(QKeySequence(QKeySequence::Save), [this] {                                           // Ctrl+S: write the .phy session
        if (m_ctx->saveSession()) showMessage(QString(" ") + m_ctx->sessionFile);
    });
    add(QKeySequence(QKeySequence::Print), [this] { openReportPage(); });                     // Ctrl+P: the PDF report mode on Export
    add(QKeySequence(Qt::CTRL | Qt::Key_Return), [this] { triggerPrimaryAction(); });
    add(QKeySequence(Qt::CTRL | Qt::Key_Enter), [this] { triggerPrimaryAction(); });    // numeric keypad
    for (int i = 1; i <= 9; ++i)                                              // Ctrl+1..9: the first nine pages
        add(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + i)), [this, i] { setCurrentPage(i - 1); });
    add(QKeySequence(Qt::CTRL | Qt::Key_Tab), [this] {                        // Ctrl+Tab: next page
        setCurrentPage((currentPage() + 1) % pageCount());
    });
}

void MainWindow::triggerPrimaryAction() {
    QWidget* page = m_stack->currentWidget();
    if (!page) return;
    for (QPushButton* b : page->findChildren<QPushButton*>()) {
        if (!b->isVisibleTo(page) || !b->isEnabled()) continue;
        const QString text = b->text();
        if (b->property("primaryAction").toBool() ||
            text == LanguageManager::instance().text("Solve") ||
            text == LanguageManager::instance().text("Evaluate")) { b->click(); return; }
    }
}

void MainWindow::openReportPage() {
    for (int i = 0; i < m_stack->count(); ++i) {
        if (auto* exportPage = qobject_cast<ExportPage*>(m_stack->widget(i))) {
            exportPage->showPdfReport();
            setCurrentPage(i);
            return;
        }
    }
}

QString MainWindow::statusLeftText() const { return m_statusLeft->text(); }

QString MainWindow::pageName(int index) const {
    for (const NavEntry& entry : m_nav)
        if (entry.page == index)
            if (const auto* button = static_cast<const NavButton*>(entry.button)) return button->title();
    return {};
}

void MainWindow::addActivity(const QString& action, const QString& detail) {
    if (!m_activityLog) return;
    m_activityEntries.insert(m_activityEntries.begin(), {action, detail, QTime::currentTime().toString("HH:mm")});
    if (m_activityEntries.size() > 3) m_activityEntries.resize(3);
    refreshActivityLog();
}

void MainWindow::refreshActivityLog() {
    if (!m_activityLog) return;
    m_activityLog->clear();
    for (const ActivityEntry& entry : m_activityEntries) {
        QString text = LanguageManager::instance().text(entry.action);
        if (!entry.detail.isEmpty()) text += QString(" ") + LanguageManager::instance().text(entry.detail);
        auto* item = new QListWidgetItem(entry.time + QString(" ") + text);
        item->setToolTip(item->text());
        m_activityLog->addItem(item);
    }
}

void MainWindow::refreshStatus() {
    BasePage::StatusInfo info;
    if (auto* page = qobject_cast<BasePage*>(m_stack->currentWidget())) info = page->statusInfo();
    m_statusLeft->setText(m_transient.isEmpty() ? info.left : m_transient);
    m_statusMid->setText(info.mid);
    m_statusRight->setText(info.right);
    for (QLabel* l : {m_statusLeft, m_statusMid, m_statusRight}) {
        l->setProperty("error", info.error);
        l->style()->unpolish(l);
        l->style()->polish(l);
    }
}

void MainWindow::showMessage(const QString& text) {
    m_transient = text;
    m_messageTimer->start();
    refreshStatus();
}

int MainWindow::pageCount() const { return m_stack->count(); }
int MainWindow::currentPage() const { return m_stack->currentIndex(); }
QWidget* MainWindow::page(int index) const { return m_stack->widget(index); }
int MainWindow::indexOf(QWidget* page) const { return m_stack->indexOf(page); }

QList<QPushButton*> MainWindow::navButtons() const {
    QList<QPushButton*> out;
    for (const NavEntry& e : m_nav) out << e.button;
    return out;
}

int MainWindow::addPage(const QString& title, const QString& icon, QWidget* page, int section) {
    const int index = m_stack->addWidget(page);
    if (auto* base = qobject_cast<BasePage*>(page)) connect(base, &BasePage::stateChanged, this, [this] { refreshStatus(); });
    addNavButton(title, icon, index, section);
    return index;
}

void MainWindow::addNavAlias(const QString& title, const QString& icon, int pageIndex, int section) {
    addNavButton(title, icon, pageIndex, section);
}

void MainWindow::addNavButton(const QString& title, const QString& icon, int pageIndex, int section) {
    const bool wraps = title.contains(" (") && title.size() > 22;
    auto* b = new NavButton(title, icon);
    b->setObjectName(section == 0 ? "navItem" : "navItemSmall");
    if (wraps) b->setMinimumHeight(54);
    m_navGroup->addButton(b);
    (section == 0 ? m_modules : m_tools)->addWidget(b);
    m_nav.push_back({b, icon, pageIndex});
    connect(b, &QPushButton::clicked, this, [this, pageIndex] { setCurrentPage(pageIndex); });
    if (m_nav.size() == 1) b->setChecked(true);
}

void MainWindow::setCurrentPage(int index) {
    if (index < 0 || index >= m_stack->count()) return;
    const bool changed = index != m_stack->currentIndex();
    m_stack->setCurrentIndex(index);
    for (const NavEntry& e : m_nav)
        if (e.page == index) { e.button->setChecked(true); break; }
    if (changed) addActivity("Opened", pageName(index));
}

void MainWindow::refreshIcons() {
    for (const NavEntry& e : m_nav) e.button->update();
}

void MainWindow::refreshLanguageButton() {
    if (!m_languageButton) return;
    const bool arabic = LanguageManager::instance().isArabic();
    m_languageButton->setText(arabic ? "English" : "العربية");
    m_languageButton->setToolTip(arabic ? "التبديل إلى الإنجليزية" : "Switch the interface to Arabic");
    m_languageButton->setAccessibleName(arabic ? "التبديل إلى الإنجليزية" : "Switch to Arabic");
}

} // namespace phy104
