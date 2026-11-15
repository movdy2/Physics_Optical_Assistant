#pragma once
#include "Models.h"

#include <QList>
#include <QMainWindow>
#include <QStringList>
#include <vector>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTimer;
class QVBoxLayout;

namespace phy104 {

class AppContext;
class BasePage;
class VirtualKeyboard;

/// Figma shell: sidebar (navigation and recent activity) | pages | status bar.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(AppContext* context, const std::vector<KeyboardGroup>& keyboardLayout, QWidget* parent = nullptr);

    /// Adds a page to the stack. `section` 0 = main module list, 1 = tools list. Returns the stack index.
    int addPage(const QString& title, const QString& icon, QWidget* page, int section = 0);
    /// A second sidebar entry that opens an existing page.
    void addNavAlias(const QString& title, const QString& icon, int pageIndex, int section = 0);

    int pageCount() const;
    int currentPage() const;
    QWidget* page(int index) const;
    int indexOf(QWidget* page) const;
    QList<QPushButton*> navButtons() const;
    QPushButton* themeButton() const { return m_themeButton; }
    QPushButton* keyboardButton() const { return m_keyboardButton; }
    QPushButton* languageButton() const { return m_languageButton; }
    VirtualKeyboard* keyboard() const { return m_keyboard; }
    QListWidget* activityLog() const { return m_activityLog; }
    QString statusLeftText() const;

    /// "Ctrl+S", "Ctrl+P", "Ctrl+Return", "Ctrl+1".."Ctrl+9", "Ctrl+Tab": what the window listens to.
    QStringList shortcutKeys() const { return m_shortcutKeys; }

public slots:
    void setCurrentPage(int index);
    /// Ctrl+Enter: the main action of the page that is showing ("Calculate & Simulate", "Solve", ...).
    void triggerPrimaryAction();
    /// Ctrl+P: open the PDF report mode on the unified Export page.
    void openReportPage();

signals:
    void themeToggled(bool dark);

private:
    void addNavButton(const QString& title, const QString& icon, int pageIndex, int section);
    void installShortcuts();
    void refreshIcons();
    void refreshLanguageButton();
    void refreshStatus();
    void showMessage(const QString& text);
    void addActivity(const QString& action, const QString& detail = QString());
    void refreshActivityLog();
    QString pageName(int index) const;
    QLineEdit* fallbackField() const;

    AppContext* m_ctx;
    QStackedWidget* m_stack;
    QVBoxLayout* m_modules;
    QVBoxLayout* m_tools;
    QLabel *m_modulesHeader, *m_toolsHeader;
    QButtonGroup* m_navGroup;
    struct NavEntry { QPushButton* button; QString icon; int page; };
    std::vector<NavEntry> m_nav;
    VirtualKeyboard* m_keyboard;
    QLabel *m_statusLeft, *m_statusMid, *m_statusRight;
    struct ActivityEntry { QString action; QString detail; QString time; };
    QListWidget* m_activityLog;
    std::vector<ActivityEntry> m_activityEntries;
    int m_lastRunCount = 0;
    QString m_transient;
    QTimer* m_messageTimer;
    QPushButton *m_themeButton, *m_keyboardButton, *m_languageButton;
    QStringList m_shortcutKeys;
};

} // namespace phy104
