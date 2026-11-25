// Drives the REAL application with simulated mouse clicks and key presses (QtTest) and checks
// that every control does something. This is the regression test for "a button does not work".
#include "App.h"
#include "ui/AppContext.h"
#include "ui/Components.h"
#include "ui/DashboardPage.h"
#include "ui/DiagramsPage.h"
#include "ui/ExperimentPages.h"
#include "ui/GuidedLabDialog.h"
#include "ui/MainWindow.h"
#include "ui/EquationLabPage.h"
#include "ui/ExportPage.h"
#include "ui/ExportService.h"
#include "ui/RecentExportPanel.h"
#include "ui/MarkdownHighlighter.h"
#include "ui/LatexRenderer.h"
#include "ui/ModelExperimentPages.h"
#include "ui/ScientificKeyboard.h"
#include "ui/MarkdownWorkspace.h"
#include "ui/MathSolverPage.h"
#include "ui/NotesPage.h"
#include "ui/OpticsCanvases.h"
#include "ui/ReportPage.h"
#include "ui/SolverPage.h"
#include "ui/StudyHubPage.h"
#include "ui/ThickLensPage.h"
#include "JsonRepositories.h"
#include "ui/ThemeManager.h"
#include "ui/LanguageManager.h"
#include "ui/VirtualKeyboard.h"

#include <QApplication>
#include <QAction>
#include <QColor>
#include <QCheckBox>
#include <QDate>
#include <QComboBox>
#include <QElapsedTimer>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QKeySequence>
#include <QSignalSpy>
#include <QListWidget>
#include <QMenu>
#include <QScrollArea>
#include <QPalette>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointF>
#include <QRawFont>
#include <QRect>
#include <QPushButton>
#include <QStandardPaths>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QTest>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTextFragment>
#include <QToolButton>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <vector>

using namespace phy104;

namespace {

QPushButton* findButton(QWidget* root, const QString& text) {
    for (auto* b : root->findChildren<QPushButton*>())
        if (b->text().trimmed() == text && b->isVisibleTo(root->window())) return b;
    return nullptr;
}

QPushButton* findControl(QWidget* root, const QString& id) {
    for (auto* button : root->findChildren<QPushButton*>())
        if (button->property("controlId").toString() == id && button->isVisibleTo(root->window())) return button;
    return nullptr;
}

void click(QWidget* w) {
    QVERIFY2(w, "widget to click not found");
    QVERIFY2(w->isVisibleTo(w->window()), qPrintable(w->metaObject()->className() + QString(" is not visible")));
    QVERIFY2(w->isEnabled(), qPrintable(w->metaObject()->className() + QString(" is disabled")));
    QTest::mouseClick(w, Qt::LeftButton, Qt::NoModifier, w->rect().center());
}

/// Types text like a keyboard does, including characters outside Latin-1 (QTest::keyClicks cannot send θ or ₁).
void typeText(QWidget* w, const QString& text) {
    for (const QChar ch : text) {
        if (ch.unicode() < 128) QTest::keyClick(w, ch.toLatin1());
        else QTest::sendKeyEvent(QTest::Click, w, Qt::Key_unknown, QString(ch), Qt::NoModifier);
    }
}

/// Click into the field, replace its text by typing (no Enter!).
void typeInto(UnitField* f, const QString& text) {
    QTest::mouseClick(f->edit(), Qt::LeftButton);
    f->edit()->selectAll();
    QTest::keyClicks(f->edit(), text);
}

QStringList allLabelTexts(QWidget* root) {
    QApplication::processEvents();                              // newly created labels are shown by the event loop
    QStringList out;
    for (auto* l : root->findChildren<QLabel*>()) if (l->isVisibleTo(root->window())) out << l->text();
    return out;
}

QString bigValue(QWidget* page, int index = 0) {
    QApplication::processEvents();
    QList<QLabel*> bigs;
    for (auto* l : page->findChildren<QLabel*>("lcdBig")) if (l->isVisibleTo(page->window())) bigs << l;
    return index < bigs.size() ? bigs[index]->text() : QString();
}

/// The real tab buttons inside a ModeTabs strip (objectName "modeTab").
QPushButton* modeTab(QWidget* page, const QString& label) {
    for (auto* b : page->findChildren<QPushButton*>(QStringLiteral("modeTab")))
        if (b->text() == label) return b;
    return nullptr;
}

QComboBox* comboWith(QWidget* page, const QString& itemText) {
    for (auto* c : page->findChildren<QComboBox*>())
        for (int i = 0; i < c->count(); ++i)
            if (c->itemText(i) == itemText) return c;
    return nullptr;
}

/// Picks an item the way the pages listen to it: `activated` is only emitted for user choices.
void chooseItem(QComboBox* c, int index) {
    QVERIFY2(c, "combo not found");
    c->setCurrentIndex(index);
    c->activated(index);
}

} // namespace

class InteractionTest : public QObject {
    Q_OBJECT
    std::unique_ptr<QTemporaryDir> m_tmp;
    std::unique_ptr<App> m_app;

    QString sessionFile() const { return m_tmp->path() + "/session.phy"; }
    MainWindow& win() { return m_app->window(); }
    void go(int page) { win().setCurrentPage(page); QApplication::processEvents(); }
    int idx(QWidget* w) { return win().indexOf(w); }
    void goTo(QWidget* w) { go(idx(w)); }

private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        LanguageManager::instance().install(*qApp);
        ThemeManager::loadFonts();
    }
    void init() {
        m_tmp = std::make_unique<QTemporaryDir>();
        QVERIFY2(m_tmp->isValid(), "could not create an isolated interaction-test directory");
        LanguageManager::instance().setLanguage(UiLanguage::English);
        ThemeManager::instance().apply(*qApp, true);
        m_app = std::make_unique<App>(App::Paths{DATA_DIR, sessionFile()});
        QObject::connect(&win(), &MainWindow::themeToggled, qApp, [](bool dark) {
            ThemeManager::instance().apply(*qApp, dark);
        });
        win().resize(1280, 800);
        win().show();
        QVERIFY(QTest::qWaitForWindowExposed(&win()));
    }
    void cleanup() {
        m_app.reset();
        m_tmp.reset();
    }
    void cleanupTestCase() { m_app.reset(); }

    // ------------------------------------------------------------------ navigation
    void sidebarButtonsOpenTheirPages() {
        const auto buttons = win().navButtons();
        QVERIFY(buttons.size() >= 10);
        auto* navigation = win().findChild<QScrollArea*>("navigationScroll");
        QVERIFY(navigation);
        for (QPushButton* b : buttons) {
            navigation->ensureWidgetVisible(b, 4, 4);
            QApplication::processEvents();
            click(b);
            QVERIFY2(b->isChecked(), qPrintable("nav button not checked: " + b->text()));
        }
        navigation->ensureWidgetVisible(buttons[1], 4, 4);
        click(buttons[1]);
        QCOMPARE(win().currentPage(), 1);
        navigation->ensureWidgetVisible(buttons[4], 4, 4);
        click(buttons[4]);
        QCOMPARE(win().currentPage(), 4);
        navigation->ensureWidgetVisible(buttons[0], 4, 4);
        click(buttons[0]);
        QCOMPARE(win().currentPage(), 0);
    }

    void shellUsesSingleV23SidebarAndKeepsActivityData() {
        QVERIFY(win().windowTitle().isEmpty());
        const QStringList labels = allLabelTexts(&win());
        for (const QString& expected : {QStringLiteral("PHY104"), QStringLiteral("Optical Assistant"),
                                        QStringLiteral("COMPUTATIONAL OPTICS / v1.4"), QStringLiteral("PAGES"),
                                        QStringLiteral("LAB SESSION"), QStringLiteral("ENGINE READY")}) {
            bool found = false;
            for (const QString& label : labels) found |= label.contains(expected, Qt::CaseInsensitive);
            QVERIFY2(found, qPrintable("missing v2.3 sidebar label: " + expected));
        }
        for (const QString& label : labels) {
            QVERIFY2(!label.contains("TOOLS", Qt::CaseInsensitive), qPrintable("extra Tools group remains: " + label));
            QVERIFY2(!label.contains("RECENT ACTIVITY", Qt::CaseInsensitive), qPrintable("activity panel remains visible: " + label));
        }
        const auto nav = win().navButtons();
        QVERIFY(!nav.isEmpty());
        QCOMPARE(nav.size(), win().pageCount());
        for (QPushButton* button : nav) QCOMPARE(button->objectName(), QString("navItem"));
        auto* savedRunCount = win().findChild<QLabel*>("sidebarCount");
        QVERIFY(savedRunCount);
        const int countBefore = savedRunCount->text().toInt();
        auto* activity = win().activityLog();
        QVERIFY(activity && activity->objectName() == "activityLog");
        QVERIFY(activity->isHidden());                                      // kept as data, not a sidebar panel
        QVERIFY(activity->count() > 0);
        goTo(m_app->refraction);
        QTRY_VERIFY_WITH_TIMEOUT(activity->item(0)->text().contains("Refraction & TIR"), 1500);
        click(findControl(m_app->refraction, "calculate"));
        QTRY_VERIFY_WITH_TIMEOUT(activity->item(0)->text().contains("#"), 1500);
        QTRY_COMPARE_WITH_TIMEOUT(savedRunCount->text().toInt(), countBefore + 1, 1500);
    }
    void guidedRefractionLabAppliesItsSuggestedSetup() {
        goTo(m_app->refraction);
        auto* launch = findButton(m_app->refraction, "Start guided lab");
        QVERIFY(launch);
        click(launch);
        auto* guide = m_app->refraction->findChild<GuidedLabDialog*>("guidedLabDialog");
        QVERIFY(guide);
        QVERIFY(!guide->isModal());
        click(findButton(guide, "Next →"));
        auto* applyValues = findButton(guide, "Use suggested values · air → glass · 30°");
        QVERIFY(applyValues);
        click(applyValues);
        const auto fields = m_app->refraction->findChildren<UnitField*>();
        QVERIFY(fields.size() >= 3);
        QCOMPARE(fields[0]->value(), 1.0);
        QCOMPARE(fields[1]->value(), 1.5);
        QCOMPARE(fields[2]->value(), 30.0);
        guide->close();
    }
    void dashboardCardsAndButtonsNavigate() {
        go(0);
        const auto cards = m_app->dashboard->findChildren<QFrame*>("cardHot");
        QCOMPARE(cards.size(), 4);
        for (int i = 0; i < 4; ++i) {
            go(0);
            click(cards[i]);
            QCOMPARE(win().currentPage(), i + 1);
        }
        go(0);
        click(findButton(m_app->dashboard, "Open notes"));
        QCOMPARE(win().currentPage(), idx(m_app->notes));
        go(0);
        click(findButton(m_app->dashboard, "Export data"));
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        auto* exportModes = m_app->exportPage->findChild<ModeTabs*>("exportModeTabs");
        QVERIFY(exportModes);
        QCOMPARE(exportModes->current(), 0);
    }
    void dashboardBrowseListsAllElevenExperiments() {
        go(0);
        auto* browse = m_app->dashboard->findChild<QToolButton*>("browseExperiments");
        QVERIFY2(browse && browse->menu(), "the dashboard has no Browse all menu");
        const auto actions = browse->menu()->actions();
        QCOMPARE(actions.size(), 11);
        QVERIFY(actions.back()->text().contains("Thick Lens & Aberration"));
        auto* count = m_app->dashboard->findChild<QLabel*>("experimentCount");
        QVERIFY(count);
        QCOMPARE(count->text(), QString("11"));
        QVERIFY(allLabelTexts(m_app->dashboard).contains("EXPERIMENTS"));
        actions.back()->trigger();
        QCOMPARE(win().currentPage(), idx(m_app->thickLens));
    }
    void dashboardComparesAndDeletesSavedRuns() {
        AppContext& ctx = m_app->context();
        const int before = ctx.log.savedCalculations();
        for (int i = 0; i < 2; ++i) {
            Snapshot snap;
            snap.experiment = "Prism sweep";
            snap.series.name = i == 0 ? "Sweep A" : "Sweep B";
            snap.series.headers = {"d_o (mm)", "d_i (mm)"};
            snap.series.rows = {{10.0, 15.0 + i}, {20.0, 12.0 + i}, {30.0, 10.0 + i}};
            ctx.publish(snap);
            ctx.recordRun(QStringLiteral("Prism sweep"), i == 0 ? "first setting" : "second setting", "n = 1.5", true);
        }
        QCOMPARE(ctx.log.savedCalculations(), before + 2);
        const int newest = ctx.log.runs().back().id;
        QVERIFY(ctx.log.runs().back().id > 0 && ctx.log.runs().back().series.rows.size() == 3);   // the sweep travelled with the run
        go(0);

        // The dashboard's comparison panel overlays checked saved runs without a separate tab.
        auto* comparisonFilter = m_app->dashboard->findChild<QComboBox*>("comparisonFilter");
        QVERIFY(comparisonFilter);
        chooseItem(comparisonFilter, comparisonFilter->findData(QStringLiteral("Prism sweep")));
        QList<QCheckBox*> selectedRuns;
        for (auto* check : m_app->dashboard->findChildren<QCheckBox*>())
            if (check->property("runId").isValid()) selectedRuns << check;
        QCOMPARE(selectedRuns.size(), 2);
        QVERIFY(selectedRuns[0]->isChecked() && selectedRuns[1]->isChecked());

        // Selecting a row in the activity log enables deletion of that saved run.
        QFrame* newestRow = nullptr;
        for (auto* row : m_app->dashboard->findChildren<QFrame*>("logRow"))
            if (row->property("runId").toInt() == newest) newestRow = row;
        QVERIFY2(newestRow, "the newest run is listed in the activity log");
        click(newestRow);
        go(0);
        QPushButton* del = findButton(m_app->dashboard, "Delete selected");
        QVERIFY(del && del->isEnabled());
        click(del);
        QApplication::processEvents();
        QCOMPARE(ctx.log.savedCalculations(), before + 1);
        QVERIFY(ctx.log.runs().back().id < newest);
        QVERIFY(QFile::exists(sessionFile()));                       // the deletion was written to the .phy file
    }
    void pagesFillTheWindowInsteadOfACentredColumn() {
        win().resize(1920, 1000);                                          // wider than the old 1500 px cap
        QApplication::processEvents();
        go(0);
        auto* inner = m_app->dashboard->findChild<QWidget*>("pageInner");
        QVERIFY2(inner, "the page container is missing");
        QVERIFY2(inner->width() >= 1500 && inner->width() >= m_app->dashboard->width() - 2,
                 qPrintable(QString("page content is %1 px wide in a %2 px page").arg(inner->width()).arg(m_app->dashboard->width())));
        win().resize(1280, 800);
        QApplication::processEvents();
    }
    void keyboardShortcutsAreInstalled() {
        const QStringList keys = win().shortcutKeys();
        for (const char* sequence : {"Ctrl+S", "Ctrl+P", "Ctrl+Return", "Ctrl+1", "Ctrl+9", "Ctrl+Tab"}) {
            const QString expected = QKeySequence(sequence).toString(QKeySequence::NativeText);
            QVERIFY2(keys.contains(expected), qPrintable("missing shortcut: " + QString(sequence)));
        }
    }
    void ctrlEnterCalculatesAndCtrlPOpensTheReport() {
        go(1);                                                             // Refraction & TIR
        const int runs = m_app->context().log.savedCalculations();
        win().triggerPrimaryAction();                                      // what Ctrl+Enter does
        QCOMPARE(m_app->context().log.savedCalculations(), runs + 1);
        win().openReportPage();                                            // what Ctrl+P does
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        QVERIFY(m_app->reportPage->isVisibleTo(&win()));
    }
    void exportIsOneSidebarDestinationWithAFormatMenu() {
        int exportEntries = 0, pdfEntries = 0;
        for (QPushButton* button : win().navButtons()) {
            exportEntries += button->accessibleName() == "Export";
            pdfEntries += button->accessibleName() == "PDF Report";
        }
        QCOMPARE(exportEntries, 1);
        QCOMPARE(pdfEntries, 0);
        goTo(m_app->exportPage);
        auto* button = m_app->exportPage->findChild<QToolButton*>("exportMenu");
        QVERIFY2(button && button->menu() && button->isVisibleTo(&win()), "the unified Export page has no visible format menu");
        QVERIFY(m_app->reportPage->findChild<QToolButton*>("exportMenu") == nullptr);
        goTo(m_app->prisms);                                     // experiment screens share the same menu
        button = m_app->prisms->findChild<QToolButton*>("exportMenu");
        QVERIFY(button && button->isVisibleTo(&win()));
    }
    void theChosenThemeIsRemembered() {
        ThemeManager::instance().apply(*qApp, false);
        QCOMPARE(ThemeManager::storedDarkTheme(), false);
        ThemeManager::instance().apply(*qApp, true);
        QCOMPARE(ThemeManager::storedDarkTheme(), true);
    }
    void themeButtonTogglesRepeatedly() {
        const bool start = ThemeManager::instance().isDark();
        click(win().themeButton());
        QCOMPARE(ThemeManager::instance().isDark(), !start);
        for (int i = 0; i < 3; ++i) click(win().themeButton());
        QCOMPARE(ThemeManager::instance().isDark(), start);          // four toggles: back where we began
        QApplication::processEvents();
    }
    void saveSessionButtonWritesTheFile() {
        go(1);
        QFile::remove(sessionFile());
        click(findButton(m_app->refraction, "Save"));
        QVERIFY(QFile::exists(sessionFile()));
    }

    // ------------------------------------------------------------------ experiments
    void refractionUpdatesWhileTyping() {
        go(1);
        auto fields = m_app->refraction->findChildren<UnitField*>();
        QVERIFY(fields.size() >= 3);
        const QString before = bigValue(m_app->refraction);
        typeInto(fields[2], "40");                                       // θ₁ = 40°, NO Enter / focus change
        QTest::qWait(700);
        const QString after = bigValue(m_app->refraction);
        QVERIFY2(after != before, qPrintable("register did not update while typing: " + before + " -> " + after));
        QVERIFY2(after.startsWith("74."), qPrintable(after));
    }
    void calculateButtonSavesARun() {
        go(1);
        const int runs = m_app->context().log.savedCalculations();
        click(findControl(m_app->refraction, "calculate"));
        QCOMPARE(m_app->context().log.savedCalculations(), runs + 1);
    }
    void resetButtonRestoresDefaults() {
        go(1);
        click(findControl(m_app->refraction, "resetParameters"));
        auto fields = m_app->refraction->findChildren<UnitField*>();
        QCOMPARE(fields[2]->text(), QString("30.00"));
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->refraction), QString("48.5904°"), 1500);
    }
    void spinnerArrowsStepTheValue() {
        go(1);
        auto fields = m_app->refraction->findChildren<UnitField*>();
        const double before = fields[0]->value();
        QTest::mouseClick(fields[0]->edit(), Qt::LeftButton);
        QTest::keyClick(fields[0]->edit(), Qt::Key_Up);
        QVERIFY(fields[0]->value() > before);
        auto* up = fields[0]->findChild<QWidget*>("spinUp");             // the clickable ▲ of the ↕ control
        QVERIFY2(up, "spinner has no clickable arrows");
        const double mid = fields[0]->value();
        click(up);
        QVERIFY(fields[0]->value() > mid);
    }
    void modeTabsSwitchTheExperimentView() {
        go(1);
        click(findButton(m_app->refraction, "TIR"));
        QTest::qWait(50);
        QVERIFY2(allLabelTexts(m_app->refraction).join("|").contains("TIR", Qt::CaseSensitive), "TIR tab shows nothing");
        click(findButton(m_app->refraction, "Refraction"));
        go(2);
        click(findButton(m_app->lenses, "Mirror"));
        QTest::qWait(50);
        click(findButton(m_app->lenses, "Thin lens"));
    }
    void invalidInputPausesAndRestoreButtonWorks() {
        go(4);
        auto fields = m_app->wave->findChildren<UnitField*>();             // λ, d, L, m
        typeInto(fields[1], "0");
        QTest::qWait(700);
        auto* calc = findControl(m_app->wave, "calculate");
        QVERIFY(calc);
        QVERIFY2(!calc->isEnabled(), "Calculate must be disabled while the input is invalid");
        auto* restore = findControl(m_app->wave, "restoreInput");
        QVERIFY2(restore, "no Restore button in the invalid state");
        click(restore);
        QTest::qWait(100);
        QCOMPARE(fields[1]->text(), QString("0.250"));
        QVERIFY(findControl(m_app->wave, "calculate")->isEnabled());
    }

    void prismDispersionShowsSevenWavelengthDependentColours() {
        goTo(m_app->prisms);
        click(findControl(m_app->prisms, "resetParameters"));
        auto* canvas = m_app->prisms->findChild<PrismCanvas*>();
        QVERIFY(canvas);
        QCOMPARE(canvas->spectrumPaths().size(), qsizetype(0));

        QComboBox* surrounding = nullptr;
        for (QComboBox* combo : m_app->prisms->findChildren<QComboBox*>())
            if (combo->findData(QStringLiteral("air")) >= 0) { surrounding = combo; break; }
        QVERIFY(surrounding);
        const int water = surrounding->findData(QStringLiteral("water"));
        QVERIFY(water >= 0);
        chooseItem(surrounding, water);                          // test dispersion in both media
        const QString singleRayResult = bigValue(m_app->prisms);
        QVERIFY(!singleRayResult.isEmpty());

        click(modeTab(m_app->prisms, "Dispersion"));
        const auto& paths = canvas->spectrumPaths();
        QCOMPARE(paths.size(), qsizetype(7));
        const double wavelengths[] = {700.0, 620.0, 580.0, 540.0, 490.0, 450.0, 400.0};
        const QColor colours[] = {QColor("#e53935"), QColor("#fb8c00"), QColor("#fdd835"),
                                   QColor("#43a047"), QColor("#1e88e5"), QColor("#3f51b5"), QColor("#8e24aa")};
        double previousPrismIndex = 0.0;
        double previousMediumIndex = 0.0;
        for (int i = 0; i < 7; ++i) {
            QCOMPARE(paths[i].wavelengthNm, wavelengths[i]);
            QCOMPARE(paths[i].color, colours[i]);
            QVERIFY(paths[i].nPrism > previousPrismIndex);
            QVERIFY(paths[i].nMedium > previousMediumIndex);
            previousPrismIndex = paths[i].nPrism;
            previousMediumIndex = paths[i].nMedium;
        }
        QApplication::processEvents();
        QImage rendered(canvas->size(), QImage::Format_ARGB32_Premultiplied);
        rendered.fill(Qt::transparent);
        canvas->render(&rendered);
        for (const QColor& expected : colours) {
            bool visible = false;
            for (int y = 0; y < rendered.height() && !visible; ++y) {
                for (int x = 0; x < rendered.width(); ++x) {
                    const QColor actual = rendered.pixelColor(x, y);
                    if (std::abs(actual.red() - expected.red()) <= 24 &&
                        std::abs(actual.green() - expected.green()) <= 24 &&
                        std::abs(actual.blue() - expected.blue()) <= 24) {
                        visible = true;
                        break;
                    }
                }
            }
            QVERIFY2(visible, qPrintable(QString("spectrum colour %1 is not painted").arg(expected.name())));
        }

        click(modeTab(m_app->prisms, "Single ray"));
        QCOMPARE(canvas->spectrumPaths().size(), qsizetype(0));
        QCOMPARE(bigValue(m_app->prisms), singleRayResult);      // dispersion does not alter the single-ray calculation
        click(findControl(m_app->prisms, "resetParameters"));
    }

    // ------------------------------------------------------------------ calculator
    void calculatorEnterEqualsAndKeypad() {
        goTo(m_app->calculator);
        auto* input = m_app->calculator->findChild<QLineEdit*>("lcdInput");
        QVERIFY(input);
        click(input);
        input->clear();
        QTest::keyClicks(input, "2+3*4");
        QTest::keyClick(input, Qt::Key_Return);
        QCOMPARE(m_app->calculator->findChild<QLabel*>("lcdResult")->text(), QString("14"));

        input->clear();
        // on-screen keypad (same theme): 7 + 8 = 15
        for (const char* key : {"7", "+", "8"}) click(findButton(m_app->calculator, key));
        QCOMPARE(input->text(), QString("7+8"));
        click(findButton(m_app->calculator, "="));
        QCOMPARE(m_app->calculator->findChild<QLabel*>("lcdResult")->text(), QString("15"));
        click(findButton(m_app->calculator, "AC"));
        QVERIFY(input->text().isEmpty());
        click(findButton(m_app->calculator, "sin"));
        QVERIFY(input->text().startsWith("sin("));
        click(findButton(m_app->calculator, "DEL"));
        QVERIFY(!input->text().endsWith("("));
    }

    void calculatorUsesNormalGruvboxTypography() {
        goTo(m_app->calculator);
        auto* input = m_app->calculator->findChild<QLineEdit*>("lcdInput");
        auto* result = m_app->calculator->findChild<QLabel*>("lcdResult");
        QVERIFY(input && result);
        QCOMPARE(input->font().families(), qApp->font().families());
        QCOMPARE(result->font().families(), qApp->font().families());
        QVERIFY(input->font().families().contains("Inter"));

        goTo(m_app->solver);
        auto* solverInput = m_app->solver->findChild<QLineEdit*>("lcdInput");
        QVERIFY(solverInput);
        QCOMPARE(solverInput->font().families(), qApp->font().families());
    }

    // ------------------------------------------------------------------ formula solver, constants
    void formulaSolverSolvesFromTheUi() {
        goTo(m_app->solver);
        m_app->solver->selectFormula("snell_law");
        auto edits = m_app->solver->findChildren<QLineEdit*>("lcdInput");
        QVERIFY(edits.size() >= 4);
        edits[0]->setText("1"); edits[1]->setText("30"); edits[2]->setText("1.5"); edits[3]->clear();
        click(findButton(m_app->solver, "Solve"));
        const QString all = allLabelTexts(m_app->solver).join("|");
        QVERIFY2(all.contains("Solved"), "Solve showed no success status");
    }
    // ------------------------------------------------------------------ bottom-bar keyboard removal
    void globalVirtualKeyboardIsAbsentFromTheBottomBar() {
        goTo(m_app->solver);
        auto* status = win().statusBar();
        QVERIFY(status);
        QVERIFY2(!findButton(status, "⌨ Keyboard"), "the bottom-bar keyboard control is still present");
        for (QPushButton* button : status->findChildren<QPushButton*>())
            QVERIFY2(!button->text().contains("Keyboard", Qt::CaseInsensitive),
                     qPrintable("unexpected keyboard control in the status bar: " + button->text()));
        QVERIFY2(win().findChildren<VirtualKeyboard*>().isEmpty(), "the global keyboard panel is still attached to the window");
    }

    // ------------------------------------------------------------------ study workspace (Markdown notes)
    MarkdownWorkspace* workspace() { goTo(m_app->notes); return m_app->notes->findChild<MarkdownWorkspace*>(); }
    QToolButton* tool(MarkdownWorkspace* w, const QString& text) {
        for (auto* b : w->findChildren<QToolButton*>()) if (b->text() == text && b->isVisibleTo(w->window())) return b;
        return nullptr;
    }
    void clickTool(MarkdownWorkspace* w, const QString& text) {
        QToolButton* b = tool(w, text);
        QVERIFY2(b, qPrintable("toolbar button not found: " + text));
        click(b);
    }
    void builtinStarterNotesTranslateButUserContentStaysUntouched() {
        auto* w = workspace();
        const QString english = MarkdownWorkspace::starterNotes();
        QVERIFY2(w->markdown() == english,
                 qPrintable(QStringLiteral("initial starter mismatch: ") + w->markdown()));
        QVERIFY2(w->editor()->property("i18nBuiltinContent").toBool(), "the starter note lost its built-in marker");
        auto* language = win().languageButton();
        click(language);
        const QString arabic = QString::fromUtf8(
            "# ملاحظات المختبر\n\n## الانكسار والانعكاس الكلي الداخلي\n\n### قانون سنيل\n"
            "- $n₁ sin θ₁ = n₂ sin θ₂$\n- قارن معامل الانكسار المستخرج بالقيمة المرجعية\n\n"
            "#### تنبيه للاختبار\nتُقاس الزوايا من العمود المقام، لا من السطح.\n");
        QVERIFY2(LanguageManager::instance().text(english) == arabic,
                 qPrintable(QStringLiteral("starter catalog text mismatch: ") + LanguageManager::instance().text(english)));
        QVERIFY2(w->markdown() == arabic,
                 qPrintable(QStringLiteral("Arabic starter mismatch (isArabic=%1, marker=%2, source=%3): %4")
                                .arg(LanguageManager::instance().isArabic())
                                .arg(w->editor()->property("i18nBuiltinContent").toBool())
                                .arg(w->editor()->property("i18nBuiltinSource").toString(), w->markdown())));
        click(language);
        QVERIFY2(w->markdown() == english,
                 qPrintable(QStringLiteral("starter did not return to English: ") + w->markdown()));

        const QString authored = "# My notes\n- n₁ sin θ₁ = n₂ sin θ₂\n";
        w->setMarkdown(authored); // loading student text clears the built-in-content marker
        click(language);
        QVERIFY2(w->markdown() == authored,
                 qPrintable(QStringLiteral("user notes changed on Arabic switch: ") + w->markdown()));
        click(language);
        QVERIFY2(w->markdown() == authored,
                 qPrintable(QStringLiteral("user notes changed on English switch: ") + w->markdown()));
    }
    void workspaceHasSnippetToolbar() {
        auto* w = workspace();
        QVERIFY(w);
        w->setMode(MarkdownWorkspace::Split);
        for (const char* label : {"θ", "λ", "δ", "n₁", "n₂", "sin", "arcsin", "$ $", "H1", "H2", "H3", "H4 ⚠", "•", "Problem template", "Formulas ▾"})
            QVERIFY2(tool(w, label), label);
    }
    void snippetButtonsInsertAtTheCursor() {
        auto* w = workspace();
        w->setMarkdown("");
        w->editor()->setFocus();
        clickTool(w, "θ");
        clickTool(w, "n₁");
        QCOMPARE(w->markdown(), QString("θn₁"));
        w->setMarkdown("");
        clickTool(w, "sin");
        QCOMPARE(w->markdown(), QString("sin()"));
        QCOMPARE(w->editor()->textCursor().position(), 4);                  // cursor between the brackets
        typeText(w->editor(), QString::fromUtf8("θ"));
        QCOMPARE(w->markdown(), QString("sin(θ)"));
        w->setMarkdown("n1 sin t1");
        QTextCursor c = w->editor()->textCursor();
        c.select(QTextCursor::Document);
        w->editor()->setTextCursor(c);
        clickTool(w, "$ $");
        QCOMPARE(w->markdown(), QString("$n1 sin t1$"));                    // wraps the selection
        clickTool(w, "arcsin");
        QVERIFY(w->markdown().contains("arcsin("));
    }
    void headingButtonsSetAndToggleTheLevel() {
        auto* w = workspace();
        w->setMarkdown("Snell's law");
        w->editor()->setFocus();
        clickTool(w, "H3");
        QCOMPARE(w->markdown(), QString("### Snell's law"));
        clickTool(w, "H1");
        QCOMPARE(w->markdown(), QString("# Snell's law"));
        clickTool(w, "H1");
        QCOMPARE(w->markdown(), QString("Snell's law"));                    // same level again removes it
        clickTool(w, "H4 ⚠");
        QCOMPARE(w->markdown(), QString("#### Snell's law"));
        clickTool(w, "•");
        clickTool(w, "•");
    }
    void problemTemplateInsertsTheThreeSections() {
        auto* w = workspace();
        w->setMarkdown("");
        w->editor()->setFocus();
        clickTool(w, "Problem template");
        const QString text = w->markdown();
        QVERIFY(text.contains("### Given:") && text.contains("### Formula:") && text.contains("### Substitution & Solution:"));
        QVERIFY(text.indexOf("### Given:") < text.indexOf("### Formula:") && text.indexOf("### Formula:") < text.indexOf("### Substitution & Solution:"));
        QCOMPARE(w->editor()->textCursor().block().text(), QString("- "));        // ready to type the first given value
        typeText(w->editor(), QString::fromUtf8("n₁ = 1.5"));
        QVERIFY(w->markdown().contains("- n₁ = 1.5"));
        click(tool(w, "Problem template"));                                       // a second one starts on its own line
        QCOMPARE(w->markdown().count("### Given:"), 2);
    }
    void enterContinuesAndEndsBulletLists() {
        auto* w = workspace();
        w->setMarkdown("");
        w->editor()->setFocus();
        typeText(w->editor(), QString::fromUtf8("- first"));
        QTest::keyClick(w->editor(), Qt::Key_Return);
        QCOMPARE(w->markdown(), QString("- first\n- "));
        typeText(w->editor(), QString::fromUtf8("second"));
        QTest::keyClick(w->editor(), Qt::Key_Return);
        QTest::keyClick(w->editor(), Qt::Key_Return);                             // empty item ends the list
        QCOMPARE(w->markdown(), QString("- first\n- second\n"));
    }
    void editorIsColourCoded() {
        auto* w = workspace();
        w->setMarkdown("# Title\n## Chapter\n### Section\n#### Warning\n- item with $x$\n");
        QApplication::processEvents();
        auto* doc = w->editor()->document();
        auto colorOf = [&](int block, int column = 0) {
            const auto formats = doc->findBlockByNumber(block).layout()->formats();
            for (const auto& r : formats) if (r.start <= column && column < r.start + r.length) return r.format.foreground().color();
            return QColor();
        };
        QCOMPARE(colorOf(0), QColor("#d79921"));
        QCOMPARE(colorOf(1), QColor("#e78a4e"));
        QCOMPARE(colorOf(2), QColor("#a9b665"));
        QCOMPARE(colorOf(3), QColor("#ea696c"));
        QCOMPARE(colorOf(4, 2), QColor("#ebdbb2"));                                  // bullet text
        QCOMPARE(colorOf(4, 14), QColor("#b8bb26"));                                 // the formula inside the bullet
    }
    void previewShowsHeadingsAndFormulas() {
        auto* w = workspace();
        w->setMarkdown("# Optics notes\n\n- see $n₁ sin θ₁ = n₂ sin θ₂$\n");
        click(findButton(w, "Preview"));
        QVERIFY(w->preview()->isVisible() && !w->editor()->isVisible());
        QTextBlock heading = w->preview()->document()->begin();
        QCOMPARE(heading.blockFormat().headingLevel(), 1);
        QCOMPARE(heading.begin().fragment().charFormat().foreground().color(), QColor("#d79921"));
        QCOMPARE(heading.begin().fragment().charFormat().fontPointSize(), 18.0);
        bool formulaImage = false;                                                   // the formula is a LaTeX image, not code text
        for (QTextBlock b = w->preview()->document()->begin(); b.isValid(); b = b.next())
            for (auto it = b.begin(); !it.atEnd(); ++it)
                if (it.fragment().charFormat().isImageFormat()) {
                    const QString name = it.fragment().charFormat().toImageFormat().name();
                    const QVariant res = w->preview()->document()->resource(QTextDocument::ImageResource, QUrl(name));
                    formulaImage = formulaImage || (res.isValid() && !res.value<QPixmap>().isNull());
                }
        QVERIFY2(formulaImage, "formula in the preview is not rendered as a LaTeX image");
        click(findButton(w, "Split"));
        QVERIFY(w->preview()->isVisible() && w->editor()->isVisible());
        click(findButton(w, "Edit"));
        QVERIFY(w->editor()->isVisible() && !w->preview()->isVisible());
        click(findButton(w, "Split"));
    }
    void previewKeepsImagesWithoutAltText() {
        auto* w = workspace();
        w->setMode(MarkdownWorkspace::Preview);
        w->setMarkdown("![](file:///tmp/phy104-diagram.png)");
        int images = 0;                                                       // Qt drops ![](...), so the preview must rescue it
        for (QTextBlock b = w->preview()->document()->begin(); b.isValid(); b = b.next())
            for (auto it = b.begin(); !it.atEnd(); ++it)
                if (it.fragment().charFormat().isImageFormat()) ++images;
        QCOMPARE(images, 1);
        w->setMarkdown("```\n![](not-an-image-in-a-fence.png)\n```");
        int fenced = 0;                                                       // code blocks keep their text untouched
        for (QTextBlock b = w->preview()->document()->begin(); b.isValid(); b = b.next())
            for (auto it = b.begin(); !it.atEnd(); ++it)
                if (it.fragment().charFormat().isImageFormat()) ++fenced;
        QCOMPARE(fenced, 0);
        QVERIFY(w->preview()->toPlainText().contains("![](not-an-image-in-a-fence.png)"));
        w->setMarkdown("");
        w->setMode(MarkdownWorkspace::Split);
    }
    void toolbarFromPreviewModeSwitchesToSplit() {
        auto* w = workspace();
        w->setMode(MarkdownWorkspace::Preview);
        clickTool(w, "θ");
        QCOMPARE(w->mode(), MarkdownWorkspace::Split);
        QVERIFY(w->markdown().contains("θ"));
        w->setMarkdown("");
    }

    // ------------------------------------------------------------------ notes <-> session snapshot <-> .phy file
    void typingAutosavesIntoTheSessionAndEverySnapshot() {
        auto* w = workspace();
        w->setMarkdown("");
        w->editor()->setFocus();
        QSignalSpy spy(w, &MarkdownWorkspace::contentChanged);
        typeText(w->editor(), QString::fromUtf8("# Study notes"));
        const QString typedNotes = w->markdown();
        QVERIFY2(typedNotes == "# Study notes", qPrintable(QString("editor text mismatch after typing: '%1'").arg(typedNotes)));
        QVERIFY2(spy.count() == 0, qPrintable(QString("autosave fired before debounce: %1 signal(s)").arg(spy.count())));
        QTRY_VERIFY_WITH_TIMEOUT(spy.count() == 1, 2500);
        const QString savedNotes = QString::fromStdString(m_app->context().log.notes());
        QVERIFY2(savedNotes == "# Study notes", qPrintable(QString("saved notes mismatch: '%1'").arg(savedNotes)));
        for (const char* name : {"Refraction & TIR", "Lenses & Mirrors", "Prisms", "Wave Interference"}) {
            const Snapshot* snap = m_app->context().log.snapshot(name);
            QVERIFY2(snap, name);
            const QString snapshotNotes = QString::fromStdString(snap->notesContent);
            QVERIFY2(snapshotNotes == "# Study notes",
                     qPrintable(QString("snapshot '%1' notes mismatch: '%2'").arg(QString::fromUtf8(name), snapshotNotes)));
        }
    }
    void loadingTextDoesNotTriggerAnAutosave() {
        auto* w = workspace();
        QSignalSpy spy(w, &MarkdownWorkspace::contentChanged);
        w->setMarkdown("## loaded, not typed");
        QTest::qWait(900);
        QCOMPARE(spy.count(), 0);
    }
    void flushSavesImmediately() {
        auto* w = workspace();
        w->setMarkdown("");
        w->editor()->setFocus();
        typeText(w->editor(), QString::fromUtf8("last edit before quitting"));
        w->flush();                                                                 // what aboutToQuit does
        QCOMPARE(QString::fromStdString(m_app->context().log.notes()), QString("last edit before quitting"));
    }
    void notesSurviveARestartTogetherWithTheSnapshots() {
        auto* w = workspace();
        w->setMarkdown("");
        w->editor()->setFocus();
        typeText(w->editor(), QString::fromUtf8("### Snell's law"));
        QTest::keyClick(w->editor(), Qt::Key_Return);
        typeText(w->editor(), QString::fromUtf8("- $n₁ sin θ₁ = n₂ sin θ₂$"));
        w->flush();
        click(findButton(m_app->notes, "Save"));                                   // the button of the page that is showing
        QVERIFY(QFile::exists(sessionFile()));

        App second(App::Paths{DATA_DIR, sessionFile()});                           // a fresh start reading the same file
        auto* w2 = second.notes->findChild<MarkdownWorkspace*>();
        QCOMPARE(w2->markdown(), QString("### Snell's law\n- $n₁ sin θ₁ = n₂ sin θ₂$"));
        const Snapshot* snap = second.context().log.snapshot("Refraction & TIR");
        QVERIFY(snap && !snap->registers.empty() && !snap->parameters.empty());      // parameters + calculator registers are in the snapshot
        QCOMPARE(QString::fromStdString(snap->notesContent), w2->markdown());
        QVERIFY(!snap->headline.empty());
    }


    // ------------------------------------------------------------------ the six screens of the second Figma batch
    void newScreensShowTheNumbersOfTheDesign() {
        struct Case { QWidget* page; const char* big; };
        const Case cases[] = {{m_app->newton, "1.5492 mm"}, {m_app->thinFilm, "1.0000"}, {m_app->grating, "+17.4576°"},
                              {m_app->singleSlit, "0.2865°"}, {m_app->polarization, "12.5000 W/m²"}, {m_app->michelson, "+2.3704"}};
        for (const Case& c : cases) {
            goTo(c.page);
            QTRY_COMPARE_WITH_TIMEOUT(bigValue(c.page, 0), QString::fromUtf8(c.big), 1500);
        }
        goTo(m_app->grating);
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->grating, 1), QString("7 orders"), 1500);
        goTo(m_app->singleSlit);
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->singleSlit, 1), QString("10.0001 mm"), 1500);
        goTo(m_app->newton);
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->newton, 1), QString("3.0984 mm"), 1500);
    }
    void secondTabsAndDropdownsDriveTheDiagram() {
        const auto waitForBigValue = [](QWidget* page, const QString& expected, int timeoutMs = 1500) {
            QElapsedTimer timer;
            timer.start();
            do {
                QApplication::processEvents();
                if (bigValue(page) == expected) return true;
                QTest::qWait(10);
            } while (timer.elapsed() < timeoutMs);
            return bigValue(page) == expected;
        };
        // --- thin film: the Transmission tab flips to the complementary intensity and re-captions both panels
        goTo(m_app->thinFilm);
        QVERIFY2(bigValue(m_app->thinFilm) == QString("1.0000"),
                 qPrintable(QStringLiteral("thin-film reflection result: ") + bigValue(m_app->thinFilm)));
        click(modeTab(m_app->thinFilm, "Transmission"));
        QVERIFY2(bigValue(m_app->thinFilm) == QString("0.0000"),
                 qPrintable(QStringLiteral("thin-film transmission result: ") + bigValue(m_app->thinFilm)));
        QVERIFY2(allLabelTexts(m_app->thinFilm).contains("Film layers / transmitted beam"), "diagram title did not follow the tab");
        QVERIFY(allLabelTexts(m_app->thinFilm).contains("Transmitted intensity vs thickness"));
        click(modeTab(m_app->thinFilm, "Reflection"));
        QVERIFY2(bigValue(m_app->thinFilm) == QString("1.0000"),
                 qPrintable(QStringLiteral("thin-film reflection after switching back: ") + bigValue(m_app->thinFilm)));
        QVERIFY(allLabelTexts(m_app->thinFilm).contains("Film layers / two reflected rays"));

        // --- Michelson: the Fringes tab shows the pattern numbers instead of the displacement numbers
        goTo(m_app->michelson);
        QVERIFY2(bigValue(m_app->michelson) == QString("+2.3704"),
                 qPrintable(QStringLiteral("Michelson displacement result: ") + bigValue(m_app->michelson)));
        click(modeTab(m_app->michelson, "Fringes"));
        QVERIFY2(bigValue(m_app->michelson) == QString("316.4000 nm"),
                 qPrintable(QStringLiteral("Michelson fringe spacing: ") + bigValue(m_app->michelson)));
        QVERIFY2(bigValue(m_app->michelson, 1) == QString("12.6422 fringes"),
                 qPrintable(QStringLiteral("Michelson fringe count: ") + bigValue(m_app->michelson, 1)));
        QVERIFY2(allLabelTexts(m_app->michelson).contains("Optical bench / fringe pattern"), "bench title did not follow the tab");
        // beam balance sets the contrast, not the fringe spacing
        chooseItem(comboWith(m_app->michelson, "Unequal beams · V = 0.60"), 1);
        QVERIFY2(bigValue(m_app->michelson) == QString("316.4000 nm"),
                 qPrintable(QStringLiteral("Michelson spacing after balance change: ") + bigValue(m_app->michelson)));
        QTRY_VERIFY_WITH_TIMEOUT(allLabelTexts(m_app->michelson).join(QStringLiteral(" ")).contains("contrast V = 0.60"), 1500);
        // M2 drive dropdown: piezo actuator, and the diagram says so
        chooseItem(comboWith(m_app->michelson, "Piezo actuator"), 1);
        QTRY_VERIFY_WITH_TIMEOUT(allLabelTexts(m_app->michelson).contains("Optical bench / piezo scan of M2"), 1500);
        click(modeTab(m_app->michelson, "Displacement"));
        QVERIFY2(bigValue(m_app->michelson) == QString("+2.3704"),
                 qPrintable(QStringLiteral("Michelson displacement after switching tabs: ") + bigValue(m_app->michelson)));
        click(findControl(m_app->michelson, "resetParameters"));                                  // return to the default displacement mode
        QVERIFY2(bigValue(m_app->michelson) == QString("+2.3704"),
                 qPrintable(QStringLiteral("Michelson displacement after reset: ") + bigValue(m_app->michelson)));

        // --- Michelson: dragging M2 syncs slider, field, and display ---
        goTo(m_app->michelson);
        auto* canvas = m_app->michelson->findChild<MichelsonCanvas*>(QStringLiteral("michelsonCanvas"));
        QVERIFY2(canvas, "Michelson canvas not found");
        auto* slider = m_app->michelson->findChild<QSlider*>(QStringLiteral("michelsonMirrorSlider"));
        QVERIFY2(slider, "Michelson slider not found");

        // Drag M2 rightward by +1 µm (10 px at 10 px/µm) from the default 0.75 µm → 1.75 µm
        const QPointF mirrorCenter = canvas->movableMirrorCenter();
        const QPoint pressPos = mirrorCenter.toPoint();
        QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, pressPos);
        QTest::mouseMove(canvas, pressPos + QPoint(10, 0));
        QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier, pressPos + QPoint(10, 0));
        QApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(slider->value() == 17500, 1500);

        // Slider → field/register: move slider to 0 µm → zero fringe shift
        slider->setValue(0);
        QApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->michelson) == QString("+0.0000"), 1500);

        // Slider → field/register: move slider to −1 µm → negative fringe shift
        slider->setValue(-10000);
        QApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->michelson).startsWith("-"), 1500);

        // --- Michelson wavelength mode: M1 moves vertically while legacy tabs retain M2 behavior
        click(findControl(m_app->michelson, "resetParameters"));
        click(modeTab(m_app->michelson, "Laser wavelength"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->michelson) == QString("632.8046 nm"), 1500);
        const QPoint m1Press = canvas->movableMirrorCenter().toPoint();
        QTest::mousePress(canvas, Qt::LeftButton, Qt::NoModifier, m1Press);
        QTest::mouseMove(canvas, m1Press + QPoint(0, -10));
        QTest::mouseRelease(canvas, Qt::LeftButton, Qt::NoModifier, m1Press + QPoint(0, -10));
        QTRY_VERIFY_WITH_TIMEOUT(slider->value() == 17500, 1500);
        QVERIFY2(allLabelTexts(m_app->michelson).contains("Michelson setup / virtual images M1′ and M2"),
                 "wavelength-mode setup title was not shown");
        auto* solveFor = comboWith(m_app->michelson, "Laser wavelength λ");
        QVERIFY(solveFor);
        auto wavelengthFields = m_app->michelson->findChildren<UnitField*>();
        QVERIFY(wavelengthFields.size() >= 3);
        chooseItem(solveFor, 1);
        typeInto(wavelengthFields[0], "632.8");
        const bool fringeCountUpdated = waitForBigValue(m_app->michelson, QString("5.5310 fringes"));
        QVERIFY2(fringeCountUpdated,
                 qPrintable(QStringLiteral("solve-for fringe count actual=%1, wavelength=%2 nm, displacement=%3 µm, target=%4")
                                .arg(bigValue(m_app->michelson), wavelengthFields[0]->text(),
                                     m_app->michelson->findChildren<UnitField*>()[1]->text(), solveFor->currentText())));
        chooseItem(solveFor, 2);
        typeInto(wavelengthFields[2], "5");
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->michelson) == QString("1.5820 µm"), 1500);
        QTRY_VERIFY_WITH_TIMEOUT(slider->value() == 15820, 1500);

        // --- Newton's rings: the viewing-mode dropdown now reaches the physics and the diagram
        goTo(m_app->newton);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->newton) == QString("1.5492 mm"), 1500);
        chooseItem(comboWith(m_app->newton, "Transmitted light"), 1);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->newton) == QString("1.6432 mm"), 1500);
        QVERIFY(allLabelTexts(m_app->newton).contains("Lens on plate / transmitted rings"));
        chooseItem(comboWith(m_app->newton, "Reflected light"), 0);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->newton) == QString("1.5492 mm"), 1500);
    }
    // ------------------------------------------------------------------ v2.8 experiment extras
    void thickLensScreenShowsThePrincipalPlanesAndTheAberration() {
        goTo(m_app->thickLens);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->thickLens) == QString("+10.3448 cm"), 1500);   // the focal length register
        // the magnification register (the second LCD card of the page, the first invisible one is skipped)
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->thickLens, 1) == QString("-0.5085"), 1500);    // the magnification register
        QVERIFY2(allLabelTexts(m_app->thickLens).contains("Thick element / principal planes"), "the diagram title is missing");
        QVERIFY2(m_app->thickLens->elementName() == "Biconvex (converging)", "the element type is wrong");

        // a bigger aperture makes the spherical aberration grow (△SA ∝ D²) and the notice says so
        auto fields = m_app->thickLens->findChildren<UnitField*>();                            // R₁, R₂, n, n₀, t, D, dₒ
        QVERIFY2(fields.size() >= 7, "the thick-lens fields are missing");
        const QString smallSa = allLabelTexts(m_app->thickLens).join(" ");                      // carries the D = 5 cm aberration
        typeInto(fields[5], "10");                                                             // D: 5 cm -> 10 cm
        QTest::qWait(700);
        const QString bigSa = allLabelTexts(m_app->thickLens).join(" ");
        QVERIFY(bigSa.contains("1.0417"));
        QVERIFY2(bigSa != smallSa, qPrintable(bigSa));                                          // four times the D = 5 cm value
        typeInto(fields[5], "5");
        QTest::qWait(700);

        // the Spectrum tab: one element, three colours
        click(modeTab(m_app->thickLens, "Spectrum"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->thickLens) == QString("0.1510 cm"), 1500);     // red-to-blue focal spread
        QVERIFY(allLabelTexts(m_app->thickLens).join(" ").contains("+10.0628"));              // red focus at 656 nm
        QVERIFY2(allLabelTexts(m_app->thickLens).contains("Thick element / three colours"), "the spectrum title is missing");
        click(modeTab(m_app->thickLens, "Thick lens"));
        click(findControl(m_app->thickLens, "resetParameters"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->thickLens) == QString("+10.3448 cm"), 1500);
    }
    void filmColoursTabPaintsTheSwatchOfThePhysics() {
        goTo(m_app->thinFilm);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->thinFilm) == QString("1.0000"), 1500);
        click(modeTab(m_app->thinFilm, "Colours"));
        QTRY_VERIFY_WITH_TIMEOUT(allLabelTexts(m_app->thinFilm).contains("Film layers / colour by thickness"), 1500);
        QString swatch = m_app->thinFilm->colourSwatch();
        QVERIFY2(swatch.startsWith('#') && swatch.size() == 7, "no swatch colour for the film");
        QTRY_VERIFY_WITH_TIMEOUT(allLabelTexts(m_app->thinFilm).join(" ").contains("600 nm dominant"), 1500);
        click(findControl(m_app->thinFilm, "resetParameters"));                              // back to the reflection tab
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->thinFilm) == QString("1.0000"), 1500);
        QVERIFY(m_app->thinFilm->colourSwatch().isEmpty());
    }
    void envelopeAndResolutionTabsCompleteTheWaveScreens() {
        // single slit: two slits of finite width put fringes inside the envelope
        goTo(m_app->singleSlit);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit) == QString("0.2865°"), 1500);
        click(modeTab(m_app->singleSlit, "Envelope"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit, 1) == QString("7 bright"), 1500);     // second register: fringe count
        QVERIFY2(allLabelTexts(m_app->singleSlit).contains("Two slits / fringes inside the envelope"), "envelope title missing");
        click(findControl(m_app->singleSlit, "resetParameters"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit) == QString("0.2865°"), 1500);

        // grating: the Resolution tab resolves 600 nm / (1 × 120 lines) in the first order
        goTo(m_app->grating);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->grating) == QString("+17.4576°"), 1500);
        click(modeTab(m_app->grating, "Resolution"));
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->grating), QString("30 nm"), 1500);            // δλ = 600 / (1 × 20 illuminated lines)
        QVERIFY2(allLabelTexts(m_app->grating).contains("Grating / two lines one Rayleigh step apart"), "resolution title missing");
        click(findControl(m_app->grating, "resetParameters"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->grating) == QString("+17.4576°"), 1500);
    }

    void remainingSecondTabsShowTheirOwnNumbers() {
        // grating: the Orders tab lists every peak instead of one selected order
        goTo(m_app->grating);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->grating) == QString("+17.4576°"), 1500);
        click(modeTab(m_app->grating, "Orders"));
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->grating), QString("7 orders"), 1500);
        QVERIFY2(allLabelTexts(m_app->grating).contains("Grating / every propagating order"), "orders title missing");
        click(modeTab(m_app->grating, "Transmission"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->grating) == QString("+17.4576°"), 1500);

        // single slit: Geometry puts the minima on the screen
        goTo(m_app->singleSlit);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit) == QString("0.2865°"), 1500);
        click(modeTab(m_app->singleSlit, "Geometry"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit) == QString("5.0001 mm"), 1500);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit, 1) == QString("7.1501 mm"), 1500);
        QVERIFY(allLabelTexts(m_app->singleSlit).contains("Slit / where the minima land"));
        click(modeTab(m_app->singleSlit, "Intensity"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->singleSlit) == QString("0.2865°"), 1500);

        // polarization: the Brewster tab replaces Malus' law with the Fresnel reflectances
        goTo(m_app->polarization);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->polarization) == QString("12.5000 W/m²"), 1500);
        click(modeTab(m_app->polarization, "Brewster"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->polarization) == QString("56.3099°"), 1500);
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->polarization, 1) == QString("0.1479"), 1500);
        QVERIFY(allLabelTexts(m_app->polarization).contains("Interface at Brewster / reflected beam s-polarized"));
        auto fields = m_app->polarization->findChildren<UnitField*>();                       // Iᵢₙ, θ, n₁, n₂
        QCOMPARE(fields.size(), 4);
        typeInto(fields[3], "1.333");                                                        // water instead of glass
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->polarization) == QString("53.1232°"), 1500);
        click(findControl(m_app->polarization, "resetParameters"));
        QTRY_VERIFY_WITH_TIMEOUT(bigValue(m_app->polarization) == QString("12.5000 W/m²"), 1500);
    }
    void newScreensHaveTheSaveExportHeader() {
        for (QWidget* page : QList<QWidget*>{m_app->newton, m_app->michelson, m_app->thinFilm, m_app->grating, m_app->singleSlit, m_app->polarization}) {
            goTo(page);
            QVERIFY2(findButton(page, "Save"), "missing Save");
            QVERIFY2(!findButton(page, "Save session"), "old header still shown");
            auto* exportButton = page->findChild<QToolButton*>("exportMenu");
            QVERIFY2(exportButton && exportButton->menu() && exportButton->menu()->actions().size() == 3,
                     "missing the export icon with PDF / Excel / CSV");
            QCOMPARE(exportButton->text(), QString("Export"));                 // the icon is labelled
        }
    }
    void mainPagesShareSaveExportHeaderWithoutUnitsTag() {
        const QList<QWidget*> pages = {m_app->dashboard, m_app->refraction, m_app->lenses, m_app->prisms, m_app->wave,
                                       m_app->polarization, m_app->singleSlit, m_app->grating, m_app->thinFilm, m_app->michelson,
                                       m_app->newton, m_app->thickLens, m_app->notes, m_app->equationLab, m_app->studyHub,
                                       m_app->solver, m_app->calculator, m_app->exportPage};
        for (QWidget* page : pages) {
            goTo(page);
            QVERIFY2(findButton(page, "Save"), "a main page is missing the shared Save action");
            QVERIFY2(!findButton(page, "Save session"), "the old Save session label remains");
            QVERIFY2(!page->findChild<QLabel*>("unitsTag"), "the shared header still shows a units tag");
            auto* exportButton = page->findChild<QToolButton*>("exportMenu");
            QVERIFY2(exportButton && exportButton->menu() && exportButton->menu()->actions().size() == 3,
                     "a main page is missing the single PDF / Excel / CSV export menu");
        }
    }
    void arabicLayoutMirrorsShellAndBundledFontCoversArabic() {
        QVERIFY(QFile::exists(QStringLiteral(":/fonts/NotoSansArabic.ttf")));
        QVERIFY(QFile::exists(QStringLiteral(":/licenses/NotoSansArabic-OFL.txt")));
        QVERIFY2(QFontDatabase::families().contains(QStringLiteral("Noto Sans Arabic")),
                 "the bundled Noto Sans Arabic font was not registered with Qt");
        const QRawFont arabicFont = QRawFont::fromFont(QFont(QStringLiteral("Noto Sans Arabic")));
        QVERIFY(arabicFont.isValid());
        QVERIFY(qApp->font().families().contains(QStringLiteral("Noto Sans Arabic")));
        QVERIFY(arabicFont.supportsCharacter(0x0627)); // alef
        QVERIFY(arabicFont.supportsCharacter(0x064A)); // yeh

        auto* sidebar = win().findChild<QFrame*>(QStringLiteral("sidebar"));
        auto* content = win().findChild<QStackedWidget*>(QStringLiteral("pageStack"));
        QVERIFY(sidebar && content);
        auto boundsInWindow = [this](QWidget* widget) {
            return QRect(widget->mapTo(&win(), QPoint(0, 0)), widget->size());
        };
        QApplication::processEvents();
        const QRect ltrSidebar = boundsInWindow(sidebar);
        const QRect ltrContent = boundsInWindow(content);
        QVERIFY2(ltrSidebar.right() < ltrContent.left(), "English layout should place the sidebar to the left of the page");

        auto panelWithTitle = [](QWidget* root, const QString& title) -> Panel* {
            for (Panel* panel : root->findChildren<Panel*>()) {
                auto* label = panel->findChild<QLabel*>("panelTitle");
                if (label && label->text() == title) return panel;
            }
            return nullptr;
        };
        goTo(m_app->refraction);
        auto* controls = panelWithTitle(m_app->refraction, QStringLiteral("Experiment controls"));
        auto* results = panelWithTitle(m_app->refraction, QStringLiteral("Results / Snell's law"));
        QVERIFY(controls && results);
        QVERIFY2(boundsInWindow(controls).right() < boundsInWindow(results).left(),
                 "English experiment columns should place controls before results");

        click(win().languageButton());
        QTRY_COMPARE_WITH_TIMEOUT(qApp->layoutDirection(), Qt::RightToLeft, 1500);
        QApplication::processEvents();
        QCOMPARE(win().layoutDirection(), Qt::RightToLeft);
        const QRect rtlSidebar = boundsInWindow(sidebar);
        const QRect rtlContent = boundsInWindow(content);
        QVERIFY2(rtlContent.right() < rtlSidebar.left(), "Arabic layout should mirror the sidebar to the right of the page");
        QVERIFY2(boundsInWindow(results).right() < boundsInWindow(controls).left(),
                 "Arabic experiment columns should mirror results and controls");
        auto* dashboardTitle = m_app->dashboard->findChild<QLabel*>("pageTitle");
        QVERIFY(dashboardTitle);
        QCOMPARE(dashboardTitle->text(), QString("لوحة التحكم"));
    }

    void languageSwitchTranslatesTheShellAndUsesRtl() {
        go(0);
        auto* language = win().languageButton();
        QVERIFY(language);
        QCOMPARE(language->text(), QString("العربية"));
        const QStringList englishUiFontStack = qApp->font().families();
        auto* calculatorInput = m_app->calculator->findChild<QLineEdit*>("lcdInput");
        QVERIFY(calculatorInput);
        const QStringList calculatorFontStack = calculatorInput->font().families();
        QVERIFY(englishUiFontStack.contains("Inter"));
        auto* notesWorkspace = m_app->notes->findChild<MarkdownWorkspace*>();
        QVERIFY(notesWorkspace);
        auto* notesEditor = notesWorkspace->editor();
        QCOMPARE(notesEditor->font().families(), englishUiFontStack);

        click(language);
        QCOMPARE(LanguageManager::instance().language(), UiLanguage::Arabic);
        QCOMPARE(qApp->layoutDirection(), Qt::RightToLeft);
        QCOMPARE(language->text(), QString("English"));
        QCOMPARE(language->accessibleName(), QString("التبديل إلى الإنجليزية"));
        QCOMPARE(qApp->font().families(), englishUiFontStack);
        QCOMPARE(calculatorInput->font().families(), calculatorFontStack);
        auto* formulaInput = m_app->solver->findChild<QLineEdit*>("lcdInput");
        auto* unitField = m_app->refraction->findChild<UnitField*>();
        QVERIFY(calculatorInput && formulaInput && unitField);
        QCOMPARE(calculatorInput->layoutDirection(), Qt::LeftToRight);
        QCOMPARE(formulaInput->layoutDirection(), Qt::LeftToRight);
        QCOMPARE(unitField->edit()->layoutDirection(), Qt::LeftToRight);
        QCOMPARE(m_app->dashboard->findChild<QLabel*>("pageTitle")->text(), QString("لوحة التحكم"));
        QVERIFY(findButton(m_app->dashboard, "حفظ"));
        auto* exportButton = m_app->dashboard->findChild<QToolButton*>("exportMenu");
        QVERIFY(exportButton);
        QCOMPARE(exportButton->text(), QString("تصدير"));
        QVERIFY2(win().windowTitle().isEmpty(), "the native window title should be removed");

        // First return to English, then select TIR and switch languages while that screen is open.
        go(0);
        click(language);
        QCOMPARE(LanguageManager::instance().language(), UiLanguage::English);
        QCOMPARE(qApp->layoutDirection(), Qt::LeftToRight);
        goTo(m_app->refraction);
        click(findButton(m_app->refraction, "TIR"));
        click(language);
        QCOMPARE(LanguageManager::instance().language(), UiLanguage::Arabic);
        QCOMPARE(qApp->layoutDirection(), Qt::RightToLeft);
        QApplication::processEvents();
        QVERIFY(win().activityLog()->item(0)->text().contains("فُتحت صفحة"));

        bool hasArabicTirResult = false;
        int checkedRegisterLabels = 0;
        for (QLabel* label : m_app->refraction->findChildren<QLabel*>()) {
            const QString objectName = label->objectName();
            if (!(objectName == "regTitle" || objectName == "regLine" || objectName == "regCaption" ||
                  objectName == "regBig" || objectName == "regFoot") || !label->isVisibleTo(m_app->refraction)) continue;
            ++checkedRegisterLabels;
            QVERIFY2(label->wordWrap(), qPrintable("Register label does not wrap: " + label->text()));
            if (objectName == "regTitle")
                QCOMPARE(label->font().families(), englishUiFontStack);
            else
                QVERIFY(label->font().families().contains("JetBrains Mono"));
            if (objectName != "regTitle") QVERIFY(label->alignment().testFlag(Qt::AlignRight));
            if (label->text().contains("الانعكاس الكلي الداخلي")) hasArabicTirResult = true;
            if (label->hasHeightForWidth())
                QVERIFY2(label->height() >= label->heightForWidth(label->width()), qPrintable("clipped register label: " + label->text()));
        }
        QVERIFY(checkedRegisterLabels > 0);
        QVERIFY2(hasArabicTirResult, "the Arabic TIR result is missing from the calculation registers");

        go(0);
        click(language);
        QCOMPARE(LanguageManager::instance().language(), UiLanguage::English);
        QCOMPARE(qApp->layoutDirection(), Qt::LeftToRight);
        QCOMPARE(language->text(), QString("العربية"));
        QCOMPARE(language->accessibleName(), QString("Switch to Arabic"));
        QVERIFY(win().activityLog()->item(0)->text().contains("Opened"));
        QCOMPARE(m_app->dashboard->findChild<QLabel*>("pageTitle")->text(), QString("Dashboard"));
        QVERIFY(findButton(m_app->dashboard, "Save"));
        QVERIFY2(win().windowTitle().isEmpty(), "the native window title should remain empty");
    }
    void calculatorExportMenuRoutesAllFormats() {
        goTo(m_app->calculator);
        auto* exportButton = m_app->calculator->findChild<QToolButton*>("exportMenu");
        QVERIFY(exportButton && exportButton->menu());
        const auto actions = exportButton->menu()->actions();
        QCOMPARE(actions.size(), 3);

        actions[0]->trigger();
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        QVERIFY(m_app->reportPage->isVisibleTo(&win()));             // PDF report pane of the same page
        actions[1]->trigger();
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        auto* formatTabs = m_app->exportPage->findChild<ModeTabs*>("seriesFormatTabs");
        auto* paneTabs = m_app->exportPage->findChild<ModeTabs*>("exportModeTabs");
        QVERIFY(formatTabs && paneTabs);
        QCOMPARE(paneTabs->current(), 0);
        QCOMPARE(formatTabs->current(), 0);                         // Excel selected before the destination opens
        actions[2]->trigger();
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        QCOMPARE(paneTabs->current(), 0);
        QCOMPARE(formatTabs->current(), 1);                         // CSV selected before the destination opens
    }

    void newScreensCalculateAndSendToNotes() {
        goTo(m_app->newton);
        const int runs = m_app->context().log.savedCalculations();
        click(findControl(m_app->newton, "calculate"));
        QCOMPARE(m_app->context().log.savedCalculations(), runs + 1);
        QCOMPARE(QString::fromStdString(m_app->context().log.runs().back().experiment), QString("Newton's Rings"));
        auto* w = workspace();
        w->setMarkdown("");
        m_app->context().setNotes("");
        goTo(m_app->newton);
        click(findButton(m_app->newton, "Send to\nNotes"));
        const QString notes = QString::fromStdString(m_app->context().log.notes());
        QVERIFY2(notes.contains("### Newton's Rings"), qPrintable(notes));
        QVERIFY(notes.contains("1.5492 mm") && notes.contains("λ = 600 nm"));
        QCOMPARE(workspace()->markdown(), notes);                                  // the notes page reloaded the appended text
    }

    void allElevenExperimentsShareLayoutAndSendToNotes() {
        const std::vector<std::pair<QWidget*, QString>> experiments{
            {m_app->refraction, "Refraction & TIR"}, {m_app->lenses, "Lenses & Mirrors"},
            {m_app->prisms, "Prisms"}, {m_app->wave, "Wave Interference"},
            {m_app->polarization, "Polarization (Malus & Brewster)"},
            {m_app->singleSlit, "Single-Slit Diffraction"}, {m_app->grating, "Diffraction Grating"},
            {m_app->thinFilm, "Thin-Film Interference"}, {m_app->michelson, "Michelson Interferometry"},
            {m_app->newton, "Newton's Rings"}, {m_app->thickLens, "Thick Lens & Aberration"}
        };
        AppContext& context = m_app->context();
        const QString previousNotes = QString::fromStdString(context.log.notes());
        context.setNotes("");
        for (const auto& [page, name] : experiments) {
            goTo(page);
            auto* title = page->findChild<QLabel*>("pageTitle");
            auto* subtitle = page->findChild<QLabel*>("pageSubtitle");
            QVERIFY(title && subtitle);
            QVERIFY2(title->text() == name,
                     qPrintable(QStringLiteral("%1 page title was '%2'").arg(name, title->text())));
            QVERIFY2(!subtitle->text().contains("EXPERIMENT", Qt::CaseInsensitive), qPrintable(subtitle->text()));
            auto* calculate = findControl(page, "calculate");
            auto* send = page->findChild<QPushButton*>("sendToNotes");
            QVERIFY(calculate && send && send->isVisibleTo(&win()));
            QVERIFY2(calculate->geometry().top() == send->geometry().top(),
                     qPrintable(QStringLiteral("%1 action row geometry: calculate y=%2, notes y=%3")
                                    .arg(name).arg(calculate->geometry().top()).arg(send->geometry().top())));

            auto* reset = findControl(page, "resetParameters");
            QVERIFY(reset);
            click(reset);
            const int before = context.log.savedCalculations();
            click(calculate);
            QVERIFY2(context.log.savedCalculations() == before + 1,
                     qPrintable(QStringLiteral("%1 saved-count delta: before=%2 after=%3")
                                    .arg(name).arg(before).arg(context.log.savedCalculations())));
            click(send);
            const QString notes = QString::fromStdString(context.log.notes());
            QVERIFY2(notes.contains("### " + name), qPrintable("notes missing for " + name + ": " + notes));
        }
        context.setNotes(previousNotes);
        workspace()->setMarkdown(previousNotes);
    }

    void newScreensPauseOnInvalidInputAndRestore() {
        goTo(m_app->newton);
        auto fields = m_app->newton->findChildren<UnitField*>();                    // λ, R, m
        auto* calculate = findControl(m_app->newton, "calculate");
        QVERIFY(calculate);
        typeInto(fields[2], "2.5");                                                  // order must be an integer
        QTRY_VERIFY_WITH_TIMEOUT(!calculate->isEnabled(), 1500);
        auto* restore = findControl(m_app->newton, "restoreInput");
        QVERIFY2(restore, "no Restore button");
        QVERIFY(m_app->newton->isPaused());
        QVERIFY(!win().findChild<QLabel*>("engineReady"));
        click(restore);
        QTRY_VERIFY_WITH_TIMEOUT(calculate->isEnabled(), 1500);
        QCOMPARE(fields[2]->text(), QString("4"));
        QVERIFY(!win().findChild<QLabel*>("engineReady"));

        goTo(m_app->grating);
        UnitField* gratingOrder = nullptr;
        for (auto* field : m_app->grating->findChildren<UnitField*>())
            if (field->property("fieldId").toString() == QLatin1String("m")) gratingOrder = field;
        QVERIFY(gratingOrder);
        typeInto(gratingOrder, "4");                                                // order 4 has no real angle
        QTRY_VERIFY_WITH_TIMEOUT(m_app->grating->isPaused(), 1500);
        click(findControl(m_app->grating, "restoreInput"));
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->grating, 0), QString("+17.4576°"), 1500);
    }
    void gratingSpinnerSteps() {
        goTo(m_app->grating);
        UnitField* gratingOrder = nullptr;
        for (auto* field : m_app->grating->findChildren<UnitField*>())
            if (field->property("fieldId").toString() == QLatin1String("m")) gratingOrder = field;
        QVERIFY(gratingOrder);
        click(gratingOrder->findChild<QWidget*>("spinUp"));
        QCOMPARE(gratingOrder->text(), QString("+2"));
        QTRY_COMPARE_WITH_TIMEOUT(bigValue(m_app->grating, 0), QString("+36.8699°"), 1500);
        click(gratingOrder->findChild<QWidget*>("spinDown"));
        click(m_app->grating->findChild<QPushButton*>());                            // harmless: any button
    }

    // ------------------------------------------------------------------ scientific keyboard + Equation Lab
    QPushButton* sciKey(ScientificKeyboard* k, const QString& legend) {
        for (auto* b : k->findChildren<QPushButton*>())
            if (b->property("legend").toString() == legend && b->isVisibleTo(k->window())) return b;
        return nullptr;
    }
    void equationLabEvaluatesTheDesignExample() {
        goTo(m_app->equationLab);
        auto* lab = m_app->equationLab;
        lab->source()->setPlainText("r_m = \\sqrt{\\frac{m\\lambda R}{n}}");
        QTRY_COMPARE_WITH_TIMEOUT(lab->resultText(), QString("r_m = 1.5492 mm"), 2000);
        QCOMPARE(lab->variablesTable()->rowCount(), 5);                              // r_m, m, lambda, R, n
        auto* m = lab->variablesTable()->item(1, 1);
        QCOMPARE(lab->variablesTable()->item(1, 0)->text(), QString("m"));
        m->setText("9");                                                              // edit a known value
        QTRY_COMPARE_WITH_TIMEOUT(lab->resultText(), QString("r_m = 2.3238 mm"), 2000);
        lab->source()->setPlainText("\\frac{1}{s} + \\frac{1}{s'} = \\frac{1}{f}");
        QTRY_VERIFY_WITH_TIMEOUT(lab->parsed(), 2000);
        lab->source()->setPlainText("\\frac{1}");
        QTRY_VERIFY_WITH_TIMEOUT(!lab->parsed(), 2000);                              // syntax error is reported, not a crash
    }
    void scientificKeyboardWorksLikeTheDesign() {
        goTo(m_app->equationLab);
        auto* lab = m_app->equationLab;
        auto* kb = lab->keyboard();
        QToolButton* toggle = nullptr;
        for (auto* b : lab->findChildren<QToolButton*>("fmtAccent")) toggle = b;
        QVERIFY(toggle);
        click(toggle);
        QVERIFY(kb->isVisible());
        lab->source()->setPlainText("");
        QApplication::processEvents();
        click(sciKey(kb, "λ"));                                                       // Greek page is first
        QCOMPARE(kb->pending(), QString("\\lambda"));
        QCOMPARE(lab->source()->toPlainText(), QString());                            // nothing is inserted until INSERT
        click(findButton(kb, "INSERT / ENTER  ↵"));
        QCOMPARE(lab->source()->toPlainText(), QString("\\lambda"));
        QVERIFY(kb->pending().isEmpty());
        QVERIFY2(lab->source()->hasFocus(), "focus must stay in the source editor");

        click(findButton(kb, "Math operators"));                                      // braces place the caret inside
        click(sciKey(kb, "√"));
        click(findButton(kb, "INSERT / ENTER  ↵"));
        QCOMPARE(lab->source()->toPlainText(), QString("\\lambda\\sqrt{}"));
        QCOMPARE(lab->source()->textCursor().position(), lab->source()->toPlainText().size() - 1);
        QTest::keyClick(lab->source(), 'x');
        QCOMPARE(lab->source()->toPlainText(), QString("\\lambda\\sqrt{x}"));
        click(sciKey(kb, "/"));                                                       // fraction: caret in the numerator
        QCOMPARE(kb->pending(), QString("\\frac{}{}"));
        click(findButton(kb, "AC"));
        QVERIFY2(kb->pending().isEmpty() && lab->source()->toPlainText() == "\\lambda\\sqrt{x}", "AC clears the buffer, not the equation");

        click(sciKey(kb, "2"));
        click(sciKey(kb, "xʸ"));
        QCOMPARE(kb->pending(), QString("2^{}"));
        click(findButton(kb, "⌫"));                                                   // removes the last pending token
        QCOMPARE(kb->pending(), QString("2"));
        click(findButton(kb, "⌫"));
        QVERIFY(kb->pending().isEmpty());
        const int before = lab->source()->toPlainText().size();
        click(findButton(kb, "⌫"));                                                   // empty buffer: deletes in the source
        QCOMPARE(lab->source()->toPlainText().size(), before - 1);

        const int pos = lab->source()->textCursor().position();
        click(findButton(kb, "←"));
        QCOMPARE(lab->source()->textCursor().position(), pos - 1);
        click(findButton(kb, "→"));
        QCOMPARE(lab->source()->textCursor().position(), pos);

        click(findButton(kb, "Greek symbols"));                                       // SHIFT latches capital Greek
        click(findButton(kb, "SHIFT"));
        QVERIFY(kb->shiftLatched());
        QVERIFY2(sciKey(kb, "δ") && sciKey(kb, "δ")->text() == "Δ", "SHIFT shows capital legends");
        click(sciKey(kb, "δ"));
        QCOMPARE(kb->pending(), QString::fromUtf8("Δ"));
        click(findButton(kb, "SHIFT"));
        click(findButton(kb, "AC"));

        click(findButton(kb, "Functions"));
        click(sciKey(kb, "sin"));
        QCOMPARE(kb->pending(), QString("\\sin()"));
        click(findButton(kb, "AC"));
        click(findButton(kb, "Variables"));                                            // all variables of variables.json
        int variableKeys = 0;
        for (auto* b : kb->findChildren<QPushButton*>("rkey")) if (!b->icon().isNull()) ++variableKeys;
        QVERIFY2(variableKeys >= 55, qPrintable(QString::number(variableKeys)));

        kb->setFocus();
        QTest::keyClick(kb, Qt::Key_Escape);                                           // Esc closes without inserting
        QVERIFY(!kb->isVisible() && !toggle->isChecked());
    }

    // ------------------------------------------------------------------ the export icon of the header (PDF / Excel / CSV)
    void exportIconOpensTheChosenFormat() {
        goTo(m_app->prisms);
        auto* button = m_app->prisms->findChild<QToolButton*>("exportMenu");
        QVERIFY2(button && button->menu(), "the header has no export icon");
        auto actions = button->menu()->actions();
        QCOMPARE(actions.size(), 3);
        QCOMPARE(actions[0]->text(), QString("PDF report…"));
        QCOMPARE(actions[1]->text(), QString("Excel (.xls)…"));
        QCOMPARE(actions[2]->text(), QString("CSV (.csv)…"));

        actions[1]->trigger();                                              // Excel
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        QVERIFY(findButton(m_app->exportPage, "Export workbook"));
        goTo(m_app->prisms);
        actions[2]->trigger();                                              // CSV
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        QVERIFY(findButton(m_app->exportPage, "Export CSV"));               // the format was preselected
        goTo(m_app->prisms);
        actions[0]->trigger();                                              // PDF report pane of the same Export page
        QCOMPARE(win().currentPage(), idx(m_app->exportPage));
        auto* panes = m_app->exportPage->findChild<ModeTabs*>("exportModeTabs");
        QVERIFY(panes);
        QCOMPARE(panes->current(), 1);
        QVERIFY(m_app->reportPage->isVisibleTo(&win()));
    }
    void exportPageCanPrintTheTable() {
        goTo(m_app->exportPage);
        m_app->exportPage->showDataExport();
        QVERIFY2(findButton(m_app->exportPage, "Print…"), "no Print button on the series-export pane");
    }
    // ------------------------------------------------------------------ Study Hub (JSON array -> page)
    void studyHubKeepsChapterConstantsAndRemovesGlobalConstantsTab() {
        bool hasHub = false, hasNotes = false, hasConstants = false, hasSetups = false;
        for (auto* b : win().navButtons()) {
            const QString name = b->accessibleName();
            hasHub |= name == "Study Hub";
            hasNotes |= name == "Notes";
            hasConstants |= name == "Constants";
            hasSetups |= name == "Optical Setups";
        }
        QVERIFY2(hasHub && hasNotes, "Study Hub or the Notes shortcut is missing from the sidebar");
        QVERIFY2(!hasConstants && !hasSetups, "Constants and Optical Setups should be accessed through Study Hub");

        goTo(m_app->studyHub);
        auto* sectionTabs = m_app->studyHub->findChild<QWidget*>("studyHubTabs");
        QVERIFY(sectionTabs);
        const auto tabs = sectionTabs->findChildren<QPushButton*>("modeTab");
        QCOMPARE(tabs.size(), 3);
        QCOMPARE(tabs[0]->text(), QString("Chapters"));
        QCOMPARE(tabs[1]->text(), QString("Notes"));
        QCOMPARE(tabs[2]->text(), QString("Optical Setups"));

        m_app->studyHub->selectSection(0);
        auto* list = m_app->studyHub->topicList();
        QCOMPARE(list->count(), m_app->studyHub->topicCount());
        QVERIFY2(list->count() >= 8, "the Study Hub chapter list is empty");
        QVERIFY(!m_app->studyHub->summaryLabel()->text().isEmpty());
        QVERIFY2(m_app->studyHub->constantTable()->rowCount() > 0, "no constants for the first chapter");
        auto* ruleContent = m_app->studyHub->findChild<QWidget*>("rulesContent");
        QVERIFY2(ruleContent && !ruleContent->findChildren<LatexWidget*>().isEmpty(),
                 "the chapter Rules panel has no formulas");
        bool hasChapterConstants = false;
        for (auto* label : m_app->studyHub->findChildren<QLabel*>())
            hasChapterConstants |= label->text() == "Constants";
        QVERIFY2(hasChapterConstants, "the chapter Constants panel is missing");

        list->setCurrentRow(1);
        QCOMPARE(list->currentRow(), 1);
        QVERIFY2(!m_app->studyHub->summaryLabel()->text().isEmpty(), "the second chapter has no summary on screen");

        m_app->studyHub->selectSection(1);
        auto* noteViews = m_app->studyHub->findChild<QStackedWidget*>("studyHubNoteViews");
        auto* materialList = m_app->studyHub->findChild<QListWidget*>("studyMaterialList");
        QVERIFY(noteViews && materialList);
        if (noteViews->currentIndex() != 0) click(modeTab(m_app->studyHub, "Study material"));
        QCOMPARE(materialList->count(), 20);
        QVERIFY2(materialList->isVisibleTo(&win()), "the structured study-material catalog is not visible");
        click(modeTab(m_app->studyHub, "My notes"));
        QVERIFY2(m_app->studyHub->notesEditor() && m_app->studyHub->notesEditor()->isVisibleTo(&win()), "the chapter JSON notes editor is missing");
        m_app->studyHub->selectSection(2);
        auto* diagrams = m_app->diagrams->findChild<QListWidget*>("diagramList");
        QVERIFY2(diagrams, "the Optical Setups gallery is missing from Study Hub");
        QCOMPARE(diagrams->count(), 11);
        const QStringList expectedSetups = {
            "Refraction & TIR", "Lenses & Mirrors", "Prisms", "Wave Interference (Double-Slit)",
            "Polarization (Malus & Brewster)", "Single-Slit Diffraction", "Diffraction Grating",
            "Thin-Film Interference", "Michelson Interferometry", "Newton's Rings", "Thick Lens & Aberration"
        };
        for (int i = 0; i < expectedSetups.size(); ++i)
            QCOMPARE(diagrams->item(i)->text(), expectedSetups[i]);
        auto* variants = m_app->diagrams->findChild<QComboBox*>("diagramVariantSelector");
        QVERIFY(variants);
        diagrams->setCurrentRow(0);
        QCOMPARE(variants->count(), 2);                                   // Refraction and TIR grouped as one experiment
        diagrams->setCurrentRow(2);
        QCOMPARE(variants->count(), 1);
        m_app->studyHub->selectSection(3);                                  // removed global Constants tab is ignored
        QCOMPARE(m_app->studyHub->currentSection(), 2);
        m_app->studyHub->selectSection(0);
    }

    void studyHubShowsAllStructuredNotesAndRetainsThePersonalEditor() {
        goTo(m_app->studyHub);
        m_app->studyHub->selectSection(1);
        auto* noteViews = m_app->studyHub->findChild<QStackedWidget*>("studyHubNoteViews");
        auto* materialList = m_app->studyHub->findChild<QListWidget*>("studyMaterialList");
        auto* search = m_app->studyHub->findChild<QLineEdit*>("studyMaterialSearch");
        auto* chapterFilter = m_app->studyHub->findChild<QComboBox*>("studyMaterialChapterFilter");
        auto* detailBody = m_app->studyHub->findChild<QWidget*>("studyMaterialDetailBody");
        auto* noteTitle = m_app->studyHub->findChild<QLabel*>("studyMaterialTitle");
        auto* noteMeta = m_app->studyHub->findChild<QLabel*>("studyMaterialMeta");
        QVERIFY(noteViews && materialList && search && chapterFilter && detailBody && noteTitle && noteMeta);
        if (noteViews->currentIndex() != 0) click(modeTab(m_app->studyHub, "Study material"));

        const auto catalog = JsonStudyNotesRepository(QString::fromUtf8(DATA_DIR) + "/notes.json").loadAll();
        QCOMPARE(static_cast<int>(catalog.size()), 20);
        QCOMPARE(materialList->count(), 20);
        QCOMPARE(chapterFilter->count(), 6);             // all chapters plus the five supplied chapter labels
        for (int chapterIndex = 1; chapterIndex < chapterFilter->count(); ++chapterIndex) {
            chapterFilter->setCurrentIndex(chapterIndex);
            QCOMPARE(materialList->count(), 4);          // four notes in each chapter
        }
        chapterFilter->setCurrentIndex(0);
        search->setText("Brewster");
        QVERIFY(materialList->count() > 0 && materialList->count() < 20); // search includes note body text
        search->clear();
        QCOMPARE(materialList->count(), 20);

        auto* bodyLayout = qobject_cast<QVBoxLayout*>(detailBody->layout());
        QVERIFY(bodyLayout);
        for (int row = 0; row < static_cast<int>(catalog.size()); ++row) {
            materialList->setCurrentRow(row);
            QApplication::processEvents();
            const StudyNote& expected = catalog[static_cast<std::size_t>(row)];
            QCOMPARE(noteTitle->text(), QString::fromStdString(expected.title));
            QCOMPARE(noteMeta->text(), QString::fromStdString(expected.chapter) + "  ·  " +
                                         QString::fromStdString(expected.type).toUpper() + "  ·  " +
                                         QString::fromStdString(expected.id));

            QStringList content;
            QStringList equations;
            for (int i = 0; i < bodyLayout->count(); ++i) {
                QWidget* item = bodyLayout->itemAt(i)->widget();
                if (auto* point = qobject_cast<QLabel*>(item); point && point->objectName() == "studyMaterialPoint")
                    content.push_back(point->property("studyNoteContent").toString());
                if (auto* scroll = qobject_cast<QScrollArea*>(item); scroll && scroll->objectName() == "studyMaterialEquationScroll") {
                    auto* formula = qobject_cast<LatexWidget*>(scroll->widget());
                    QVERIFY(formula);
                    equations.push_back(formula->property("rawEquation").toString());
                }
            }
            QStringList expectedContent;
            for (const std::string& line : expected.content) expectedContent.push_back(QString::fromStdString(line));
            QStringList expectedEquations;
            for (const std::string& equation : expected.equations) expectedEquations.push_back(QString::fromStdString(equation));
            QCOMPARE(content, expectedContent);
            QCOMPARE(equations, expectedEquations);
        }

        click(modeTab(m_app->studyHub, "My notes"));
        QVERIFY(m_app->studyHub->notesEditor()->isVisibleTo(&win()));
        QVERIFY(m_app->studyHub->noteSummaryEditor()->isVisibleTo(&win()));
    }

    void arabicLocaleLocalizesAllExperimentsAndStudyNotes() {
        go(0);
        auto* language = win().languageButton();
        QVERIFY(language);
        if (!LanguageManager::instance().isArabic()) click(language);
        QCOMPARE(LanguageManager::instance().language(), UiLanguage::Arabic);

        const QStringList arabicExperimentTitles{
            "الانكسار والانعكاس الكلي الداخلي", "العدسات والمرايا", "الموشورات",
            "تداخل الموجات (الشق المزدوج)", "الاستقطاب (مالوس وبروستر)", "حيود الشق الواحد",
            "محزوز الحيود", "تداخل الأغشية الرقيقة", "تداخل ميكلسون", "حلقات نيوتن",
            "العدسة السميكة والزيغ"};
        const QList<QWidget*> experimentPages{m_app->refraction, m_app->lenses, m_app->prisms, m_app->wave,
            m_app->polarization, m_app->singleSlit, m_app->grating, m_app->thinFilm, m_app->michelson,
            m_app->newton, m_app->thickLens};
        const auto navigation = win().navButtons();
        QVERIFY(navigation.size() >= arabicExperimentTitles.size() + 1);
        for (int i = 0; i < arabicExperimentTitles.size(); ++i) {
            QVERIFY2(navigation[i + 1]->accessibleName() == arabicExperimentTitles[i],
                     qPrintable(QStringLiteral("Arabic navigation title %1: %2")
                                    .arg(i).arg(navigation[i + 1]->accessibleName())));
            auto* title = experimentPages[i]->findChild<QLabel*>("pageTitle");
            QVERIFY(title);
            const QString expectedPageTitle = i == 3 ? QStringLiteral("تداخل الموجات") : arabicExperimentTitles[i];
            QVERIFY2(title->text() == expectedPageTitle,
                     qPrintable(QStringLiteral("Arabic page title %1: %2")
                                    .arg(i).arg(title->text())));
        }

        auto* experimentMenu = m_app->dashboard->findChild<QMenu*>("allExperimentsMenu");
        QVERIFY(experimentMenu);
        const auto actions = experimentMenu->actions();
        QCOMPARE(actions.size(), 11);
        const auto hasArabicText = [](const QString& text) {
            for (const QChar ch : text)
                if (ch.unicode() >= 0x0600 && ch.unicode() <= 0x06ff) return true;
            return false;
        };
        for (int i = 0; i < actions.size(); ++i) {
            const QString expected = QString("%1 · %2").arg(i + 1, 2, 10, QChar('0')).arg(arabicExperimentTitles[i]);
            QVERIFY2(actions[i]->text() == expected,
                     qPrintable(QStringLiteral("Arabic menu item %1: %2").arg(i).arg(actions[i]->text())));
            QVERIFY2(hasArabicText(actions[i]->toolTip()), qPrintable("Arabic experiment description missing: " + actions[i]->toolTip()));
        }

        goTo(m_app->notes);
        QCOMPARE(m_app->notes->findChild<QLabel*>("pageTitle")->text(), QString("الملاحظات"));
        QCOMPARE(m_app->notes->findChild<QLabel*>("pageSubtitle")->text(),
                 QString("مساحة ملاحظات دراسية بتنسيق Markdown / اكتب ملاحظات المختبر وحرّرها واعرضها"));
        QCOMPARE(m_app->notes->findChild<QLabel*>("panelTitle")->text(), QString("ملاحظات الجلسة"));
        auto* notesKeyboard = m_app->notes->findChild<QToolButton*>("notesKeyboard");
        QVERIFY(notesKeyboard);
        QCOMPARE(notesKeyboard->text(), QString("⌨ لوحة المفاتيح"));
        QVERIFY(hasArabicText(notesKeyboard->toolTip()));
        MarkdownWorkspace localizedWorkspace;
        LanguageManager::instance().apply(&localizedWorkspace);
        const auto workspaceLabels = localizedWorkspace.findChildren<QLabel*>();
        bool localizedWorkspaceTitle = false;
        int localizedCaptionCount = 0;
        for (QLabel* label : workspaceLabels) {
            if (label->objectName() != "caption") continue;
            ++localizedCaptionCount;
            QVERIFY2(hasArabicText(label->text()), qPrintable("untranslated Notes status label: " + label->text()));
            if (label->text() == LanguageManager::instance().text("MARKDOWN · FORMULA JOTTER"))
                localizedWorkspaceTitle = true;
        }
        QVERIFY(localizedWorkspaceTitle);
        QVERIFY(localizedCaptionCount >= 3);
        int localizedModeCount = 0;
        for (QPushButton* mode : localizedWorkspace.findChildren<QPushButton*>("modeTab")) {
            QVERIFY2(hasArabicText(mode->text()), qPrintable("untranslated Notes mode: " + mode->text()));
            ++localizedModeCount;
        }
        QCOMPARE(localizedModeCount, 3);
        QVERIFY(hasArabicText(localizedWorkspace.editor()->placeholderText()));
        for (QToolButton* button : localizedWorkspace.findChildren<QToolButton*>())
            if (!button->toolTip().isEmpty())
                QVERIFY2(hasArabicText(button->toolTip()), qPrintable("untranslated Notes toolbar help: " + button->toolTip()));
        QToolButton* formulaMenuButton = nullptr;
        for (auto* button : localizedWorkspace.findChildren<QToolButton*>("fmt"))
            if (button->menu()) { formulaMenuButton = button; break; }
        QVERIFY(formulaMenuButton && formulaMenuButton->menu());
        for (QAction* action : formulaMenuButton->menu()->actions())
            QVERIFY2(hasArabicText(action->text()), qPrintable("untranslated Notes formula label: " + action->text()));
        formulaMenuButton->menu()->actions().first()->trigger();
        QVERIFY(localizedWorkspace.markdown().contains("### قانون سنيل"));

        QFile lessonTranslations(":/i18n/ar-lessons.json");
        QVERIFY(lessonTranslations.open(QIODevice::ReadOnly));
        const QJsonObject arabicLessons = QJsonDocument::fromJson(lessonTranslations.readAll()).object();
        const auto translated = [&arabicLessons](const QString& source) {
            return arabicLessons.value(source).toString();
        };
        const auto catalog = JsonStudyNotesRepository(QString::fromUtf8(DATA_DIR) + "/notes.json").loadAll();
        QCOMPARE(static_cast<int>(catalog.size()), 20);

        goTo(m_app->studyHub);
        m_app->studyHub->selectSection(1);
        auto* materialList = m_app->studyHub->findChild<QListWidget*>("studyMaterialList");
        auto* search = m_app->studyHub->findChild<QLineEdit*>("studyMaterialSearch");
        auto* detailBody = m_app->studyHub->findChild<QWidget*>("studyMaterialDetailBody");
        auto* noteTitle = m_app->studyHub->findChild<QLabel*>("studyMaterialTitle");
        auto* noteMeta = m_app->studyHub->findChild<QLabel*>("studyMaterialMeta");
        QVERIFY(materialList && search && detailBody && noteTitle && noteMeta);
        QCOMPARE(materialList->count(), 20);
        auto* bodyLayout = qobject_cast<QVBoxLayout*>(detailBody->layout());
        QVERIFY(bodyLayout);

        for (int row = 0; row < static_cast<int>(catalog.size()); ++row) {
            materialList->setCurrentRow(row);
            QApplication::processEvents();
            const StudyNote& expected = catalog[static_cast<std::size_t>(row)];
            const QString title = QString::fromStdString(expected.title);
            const QString localizedTitle = translated(title);
            QVERIFY2(!localizedTitle.isEmpty(), qPrintable("missing Arabic study-note title: " + title));
            QCOMPARE(materialList->item(row)->text(), localizedTitle);
            QCOMPARE(noteTitle->text(), localizedTitle);

            const QString chapter = QString::fromStdString(expected.chapter);
            const QString type = QString::fromStdString(expected.type);
            const QString localizedChapter = translated(chapter);
            const QString localizedType = translated(type);
            QVERIFY(!localizedChapter.isEmpty() && !localizedType.isEmpty());
            const QString expectedMeta = localizedChapter + "  ·  " + localizedType.toUpper() + "  ·  " +
                                         QString::fromStdString(expected.id);
            QVERIFY2(noteMeta->text() == expectedMeta,
                     qPrintable(QStringLiteral("Arabic note metadata: ") + noteMeta->text()));

            QStringList shownPoints;
            QStringList rawPoints;
            QStringList shownEquations;
            for (int i = 0; i < bodyLayout->count(); ++i) {
                QWidget* item = bodyLayout->itemAt(i)->widget();
                if (auto* point = qobject_cast<QLabel*>(item); point && point->objectName() == "studyMaterialPoint") {
                    shownPoints.push_back(point->text());
                    rawPoints.push_back(point->property("studyNoteContent").toString());
                }
                if (auto* scroll = qobject_cast<QScrollArea*>(item); scroll && scroll->objectName() == "studyMaterialEquationScroll") {
                    auto* formula = qobject_cast<LatexWidget*>(scroll->widget());
                    QVERIFY(formula);
                    shownEquations.push_back(formula->property("rawEquation").toString());
                }
            }
            QStringList expectedArabicPoints;
            QStringList expectedRawPoints;
            for (const std::string& line : expected.content) {
                const QString raw = QString::fromStdString(line);
                const QString localizedLine = translated(raw);
                QVERIFY2(!localizedLine.isEmpty(), qPrintable("missing Arabic study-note paragraph: " + raw));
                expectedArabicPoints.push_back(QStringLiteral("•  ") + localizedLine);
                expectedRawPoints.push_back(raw);
            }
            QStringList expectedEquations;
            for (const std::string& equation : expected.equations)
                expectedEquations.push_back(QString::fromStdString(equation));
            QCOMPARE(shownPoints, expectedArabicPoints);
            QCOMPARE(rawPoints, expectedRawPoints);                // display localization never mutates source data
            QCOMPARE(shownEquations, expectedEquations);           // equations remain unchanged
        }

        const QString arabicSearch = translated(QString::fromStdString(catalog.front().title));
        QVERIFY(!arabicSearch.isEmpty());
        search->setText(arabicSearch);
        QCOMPARE(materialList->count(), 1);                         // Arabic text is searchable too
        search->clear();
        QCOMPARE(materialList->count(), 20);

        click(language);
        QCOMPARE(LanguageManager::instance().language(), UiLanguage::English);
    }

    void studyHubNotesArePerChapterJsonAndSeparateFromMarkdownNotes() {
        goTo(m_app->studyHub);
        m_app->studyHub->selectSection(1);
        auto* noteViews = m_app->studyHub->findChild<QStackedWidget*>("studyHubNoteViews");
        QVERIFY(noteViews);
        if (noteViews->currentIndex() != 1) click(modeTab(m_app->studyHub, "My notes"));
        const QString sessionNotes = QString::fromStdString(m_app->context().log.notes());
        auto* hubNotes = m_app->studyHub->notesEditor();
        auto* hubSummary = m_app->studyHub->noteSummaryEditor();
        auto* summarizeButton = m_app->studyHub->summarizeNoteButton();
        auto* chapter = m_app->studyHub->findChild<QComboBox*>("studyHubNoteChapter");
        auto* sidebarNotes = m_app->notes->findChild<MarkdownWorkspace*>();
        QVERIFY(hubNotes && hubSummary && summarizeButton && chapter && sidebarNotes);
        const QString sidebarText = sidebarNotes->markdown();

        const QString firstKey = chapter->currentData().toString();
        const QString originalNote = "Snell's law relates angles to refractive indices. Angles are measured from the normal. A higher refractive index bends light toward the normal.";
        hubNotes->setPlainText(originalNote);
        summarizeButton->click();                     // creates a local extractive draft; the user can edit it
        QVERIFY(!hubSummary->toPlainText().isEmpty());
        const QString firstSummary = "Snell's law links angles and refractive indices.";
        hubSummary->setPlainText(firstSummary);
        m_app->studyHub->flushStudyNotes();
        QVERIFY(QFile::exists(m_app->studyHub->notesFilePath()));
        QCOMPARE(QString::fromStdString(m_app->context().log.notes()), sessionNotes);
        QCOMPARE(sidebarNotes->markdown(), sidebarText);

        QFile file(m_app->studyHub->notesFilePath());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        QCOMPARE(root.value("format").toString(), QString("phy104-study-hub-notes"));
        QCOMPARE(root.value("version").toInt(), 2);
        const QJsonObject firstRecord = root.value("notes").toObject().value(firstKey).toObject();
        QCOMPARE(firstRecord.value("text").toString(), originalNote);
        QCOMPARE(firstRecord.value("summary").toString(), firstSummary);
        file.close();

        QVERIFY(chapter->count() > 1);
        chapter->setCurrentIndex(1);
        QCOMPARE(hubNotes->toPlainText(), QString());       // switching chapters must not copy the prior editor text
        QCOMPARE(hubSummary->toPlainText(), QString());
        QVERIFY(!summarizeButton->isEnabled());             // a blank note cannot produce a draft
        const QString secondKey = chapter->currentData().toString();
        hubNotes->setPlainText("Second chapter notes");
        QVERIFY(summarizeButton->isEnabled());
        hubSummary->setPlainText("A separate recap for chapter two.");
        m_app->studyHub->flushStudyNotes();
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QJsonObject bothNotes = QJsonDocument::fromJson(file.readAll()).object().value("notes").toObject();
        QCOMPARE(bothNotes.value(firstKey).toObject().value("text").toString(), originalNote);
        QCOMPARE(bothNotes.value(firstKey).toObject().value("summary").toString(), firstSummary);
        QCOMPARE(bothNotes.value(secondKey).toObject().value("text").toString(), QString("Second chapter notes"));
        QCOMPARE(bothNotes.value(secondKey).toObject().value("summary").toString(), QString("A separate recap for chapter two."));
        file.close();
        chapter->setCurrentIndex(0);
        QCOMPARE(hubNotes->toPlainText(), originalNote);
        QCOMPARE(hubSummary->toPlainText(), firstSummary);

        QFile::remove(m_app->studyHub->notesFilePath());      // keep this isolated test from affecting later cases
    }

    void calculatorHasACompactScientificKeypadAndHistory() {
        goTo(m_app->calculator);
        auto* input = m_app->calculator->findChild<QLineEdit*>("lcdInput");
        QVERIFY(input);
        auto* result = m_app->calculator->findChild<QLabel*>("lcdResult");
        QVERIFY(result);
        const QColor expectedPlaceholder = ThemeManager::instance().theme().dark
                                               ? QColor(Qt::white) : ThemeManager::instance().theme().text;
        QVERIFY2(input->palette().color(QPalette::PlaceholderText) == expectedPlaceholder,
                 "calculator placeholder color does not follow the Gruvbox theme");
        auto* angle = findButton(m_app->calculator, "DEG");
        QVERIFY(angle);
        click(angle);
        QVERIFY2(angle->text() == QString("RAD"), qPrintable(QStringLiteral("angle button after first toggle: ") + angle->text()));
        click(angle);
        QVERIFY2(angle->text() == QString("DEG"), qPrintable(QStringLiteral("angle button after second toggle: ") + angle->text()));
        QVERIFY2(!m_app->calculator->findChild<QComboBox*>("keysCombo"), "the extra physics-symbol browser is still on Calculator");
        QVERIFY2(findButton(m_app->calculator, "sin") && findButton(m_app->calculator, "sec") &&
                 findButton(m_app->calculator, "abs") && findButton(m_app->calculator, "exp") &&
                 findButton(m_app->calculator, "x²") && findButton(m_app->calculator, "1/x") &&
                 findButton(m_app->calculator, "nm") && findButton(m_app->calculator, "cm") &&
                 findButton(m_app->calculator, "mm") && findButton(m_app->calculator, "µm") &&
                 findButton(m_app->calculator, "Ans") &&
                 findButton(m_app->calculator, "=") && findButton(m_app->calculator, "AC") &&
                 findButton(m_app->calculator, "DEL"), "an advanced math key is missing");
        auto* keypad = m_app->calculator->findChild<CalculatorKeypad*>("calculatorKeypad");
        QVERIFY(keypad);
        QVERIFY2(keypad->findChildren<QPushButton*>().size() == 52,
                 qPrintable(QStringLiteral("calculator key count: %1").arg(keypad->findChildren<QPushButton*>().size())));
        for (QPushButton* key : keypad->findChildren<QPushButton*>())
            QVERIFY2(key->maximumHeight() <= 36, qPrintable("calculator key is still oversized: " + key->text()));

        auto* history = m_app->calculator->findChild<QListWidget*>("calcHistory");
        auto* historyEmpty = m_app->calculator->findChild<QLabel*>("calcHistoryEmpty");
        QVERIFY(history && historyEmpty && historyEmpty->isVisibleTo(&win()));
        input->setText("5");
        input->selectAll();
        click(findButton(m_app->calculator, "x²"));
        QVERIFY2(input->text() == QString("(5)^2"), qPrintable(QStringLiteral("square key inserted: ") + input->text()));
        input->setText("4");
        click(findButton(m_app->calculator, "1/x"));
        QVERIFY2(input->text() == QString("1/(4)"), qPrintable(QStringLiteral("reciprocal key inserted: ") + input->text()));
        input->clear();
        click(findButton(m_app->calculator, "sin"));
        QVERIFY2(input->text().startsWith("sin("), "the math key did not insert a plain-text function");
        QVERIFY2(input->text().indexOf('$') < 0 && !input->text().contains('\\'), "the calculator must insert plain text, not LaTeX");
        input->setText("632");
        click(findButton(m_app->calculator, "nm"));
        QCOMPARE(input->text(), QString("632*nm"));

        input->setText("2+3");
        QTest::keyClick(input, Qt::Key_Return);
        QVERIFY2(result->text() == QString("5"), qPrintable(QStringLiteral("2+3 result: ") + result->text()));
        QVERIFY2(history->count() == 1, qPrintable(QStringLiteral("history item count: %1").arg(history->count())));
        QVERIFY2(history->item(0)->data(Qt::UserRole).toString() == QString("2+3"),
                 qPrintable(QStringLiteral("history expression: ") + history->item(0)->data(Qt::UserRole).toString()));
        const QRect firstEntry = history->visualItemRect(history->item(0));
        QTest::mouseClick(history->viewport(), Qt::LeftButton, Qt::NoModifier, firstEntry.center());
        QCOMPARE(input->text(), QString("2+3"));

        auto* clearHistory = m_app->calculator->findChild<QPushButton*>("calcHistoryClear");
        QVERIFY(clearHistory);
        click(clearHistory);
        QCOMPARE(history->count(), 0);
        QVERIFY(historyEmpty->isVisibleTo(&win()));
        input->setText("Ans+1");
        QTest::keyClick(input, Qt::Key_Return);
        QCOMPARE(result->text(), QString("6"));                         // clearing history preserves Ans = 5
    }

    void notesPageHasTheScientificKeyboard() {
        goTo(m_app->notes);
        auto* toggle = m_app->notes->findChild<QToolButton*>("notesKeyboard");
        QVERIFY2(toggle, "no keyboard toggle on the notes page");
        auto* keyboard = m_app->notes->findChild<ScientificKeyboard*>();
        QVERIFY2(keyboard && !keyboard->isVisibleTo(&win()), "the keyboard starts closed");
        auto* workspace = m_app->notes->findChild<MarkdownWorkspace*>();
        workspace->editor()->moveCursor(QTextCursor::End);                 // no selection: the snippet only inserts
        const int before = workspace->markdown().size();
        click(toggle);
        QApplication::processEvents();
        QVERIFY2(keyboard->isVisibleTo(&win()), "the keyboard did not open");
        // opening the keyboard puts the caret inside a formula:  $…$
        QCOMPARE(workspace->markdown().size(), before + 2);
        click(toggle);
        QVERIFY2(!keyboard->isVisibleTo(&win()), "the keyboard did not close");
    }
    void scientificKeyboardSharesItsControlsAndTrimsDuplicateKeys() {
        goTo(m_app->notes);
        auto* keyboard = m_app->notes->findChild<ScientificKeyboard*>();
        auto* toggle = m_app->notes->findChild<QToolButton*>("notesKeyboard");
        QVERIFY(keyboard && toggle);
        QCOMPARE(keyboard->findChildren<QPushButton*>("rkey").size(), 123);
        QCOMPARE(keyboard->findChildren<QPushButton*>("rkeyCtl").size(), 5);

        click(toggle);
        click(findButton(keyboard, "SHIFT"));
        click(findButton(keyboard, QString::fromUtf8("Α")));
        QCOMPARE(keyboard->pending(), QString::fromUtf8("Α"));
        click(findButton(keyboard, "AC"));
        QVERIFY(keyboard->pending().isEmpty());
        click(findButton(keyboard, "SHIFT"));
        click(toggle);
    }

    void notesPageIsOnlyTheMarkdownWorkspace() {
        goTo(m_app->notes);
        QVERIFY(m_app->notes->findChild<MarkdownWorkspace*>());
        QVERIFY2(!findButton(m_app->notes, "Export workbook"), "the export GUI is still on the notes page");
        QStringList texts;
        for (auto* l : m_app->notes->findChildren<QLabel*>()) texts << l->text();
        QVERIFY2(!texts.contains("Formula reference"), "the formula reference is still on the notes page");
        QVERIFY2(!texts.contains("Export preview"), "the export preview is still on the notes page");
        QVERIFY2(m_app->notes->findChildren<RecentExportPanel*>().isEmpty(), "an export-history panel is still on the notes page");
        QVERIFY2(m_app->notes->findChildren<QTableWidget*>().isEmpty(), "an export table is still on the notes page");
    }

    void unifiedExportKeepsRecentHistoryInBothModes() {
        // The data and report panes each present the same session-backed latest export record.
        QCOMPARE(m_app->exportPage->findChildren<RecentExportPanel*>().size(), 2);
        QCOMPARE(m_app->reportPage->findChildren<RecentExportPanel*>().size(), 1);
        QCOMPARE(ExportService::fileStem("PHY104_refraction.CSV", "fallback"), QString("PHY104_refraction"));
        QCOMPARE(ExportService::outputPath("exports", "report.pdf", "fallback", ".pdf"),
                 QDir("exports").filePath("report.pdf"));
    }

    // ------------------------------------------------------------------ unified Export page: series data and report panes
    const Snapshot* selectedSeries() {
        auto* list = m_app->exportPage->seriesList();
        if (!list || !list->currentItem()) return nullptr;
        return m_app->context().log.snapshot(list->currentItem()->data(Qt::UserRole).toString().toStdString());
    }
    void exportPageListsEverySeriesAndDescribesIt() {
        goTo(m_app->exportPage);
        m_app->exportPage->showDataExport();
        auto* list = m_app->exportPage->seriesList();
        QVERIFY(list && list->count() > 0);
        int withRows = 0;                                                       // every published series that has rows
        for (const auto& entry : m_app->context().log.snapshots()) if (!entry.second.series.rows.empty()) ++withRows;
        QCOMPARE(list->count(), withRows);
        for (int i = 0; i < list->count(); ++i) {
            list->setCurrentRow(i);
            QApplication::processEvents();
            const Snapshot* snap = selectedSeries();
            QVERIFY2(snap && !snap->series.rows.empty(), "series without data in the list");
            const QStringList texts = allLabelTexts(m_app->exportPage);
            QVERIFY2(texts.contains(QString::fromStdString(snap->series.name)), "series name not described");
            QVERIFY2(texts.contains(QString::fromStdString(snap->series.description)), "series description missing");
        }
    }
    void exportPageShowsEveryRowOfTheChosenSeries() {
        goTo(m_app->exportPage);
        m_app->exportPage->showDataExport();
        auto* list = m_app->exportPage->seriesList();
        QVERIFY(list && list->count() > 0);
        int mostRows = 0;
        for (int i = 0; i < list->count(); ++i) {
            list->setCurrentRow(i);
            QApplication::processEvents();
            const Snapshot* snap = selectedSeries();
            QVERIFY(snap);
            QCOMPARE(m_app->exportPage->previewTable()->rowCount(), static_cast<int>(snap->series.rows.size()));
            QCOMPARE(m_app->exportPage->previewTable()->columnCount(), static_cast<int>(snap->series.headers.size()));
            mostRows = std::max(mostRows, m_app->exportPage->previewTable()->rowCount());
        }
        QVERIFY2(mostRows > 7, "the dedicated page must show every row, not the 7-row sample of the Notes page");
    }
    void exportPageWritesBothFormatsAndRemembersTheFile() {
        goTo(m_app->exportPage);
        m_app->exportPage->setFormat(true);                        // explicitly select Excel for this test
        const Snapshot* snap = selectedSeries();
        QVERIFY(snap);
        const QString base = "PHY104_" + QString::fromStdString(snap->series.id).section('_', 0, 0);
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PHY104";
        QDir(dir).removeRecursively();

        click(findButton(m_app->exportPage, "Export workbook"));
        const QString workbookPath = dir + "/" + base + ".xls";
        QVERIFY2(QFile::exists(workbookPath), qPrintable("no workbook in " + dir));
        QCOMPARE(QString::fromStdString(m_app->context().log.lastExport().path), workbookPath);
        QCOMPARE(QString::fromStdString(m_app->context().log.lastExport().format), QString("Excel (.xls)"));
        QVERIFY(allLabelTexts(m_app->exportPage).contains(base + ".xls"));       // the recent-export card
        QFile workbook(workbookPath);
        QVERIFY(workbook.open(QIODevice::ReadOnly));
        const QByteArray workbookBytes = workbook.readAll();
        QVERIFY(workbookBytes.startsWith("<?xml version=\"1.0\""));             // Excel 2003 SpreadsheetML, not renamed OOXML
        QVERIFY(workbookBytes.contains("<?mso-application progid=\"Excel.Sheet\"?>"));
        QVERIFY(workbookBytes.contains("<Worksheet ss:Name=\"Measurements\">"));
        QVERIFY(!workbookBytes.startsWith("PK"));

        click(findButton(m_app->exportPage, "Export as CSV instead"));
        QVERIFY2(QFile::exists(dir + "/" + base + ".csv"), qPrintable("no CSV in " + dir));
        QCOMPARE(QString::fromStdString(m_app->context().log.lastExport().format), QString("CSV (.csv)"));
        QVERIFY(allLabelTexts(m_app->exportPage).contains(base + ".csv"));
    }
    void recentExportSurvivesARestart() {
        goTo(m_app->exportPage);
        m_app->exportPage->setFormat(true);
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PHY104";
        QDir(dir).removeRecursively();
        click(findButton(m_app->exportPage, "Export workbook"));
        QVERIFY(m_app->context().log.hasExport());
        const QString path = QString::fromStdString(m_app->context().log.lastExport().path);
        QVERIFY(QFile::exists(path));
        App second(App::Paths{DATA_DIR, sessionFile()});
        QVERIFY(second.context().log.hasExport());
        QCOMPARE(QString::fromStdString(second.context().log.lastExport().path), path);
        QStringList texts;
        for (auto* l : second.exportPage->findChildren<QLabel*>()) texts << l->text();
        QVERIFY2(texts.contains(QFileInfo(path).fileName()), "the recent-export card forgot the file");
    }

    // ------------------------------------------------------------------ the PDF report pane of the unified Export page
    void reportPageCapturesEveryScreenAndWritesAPdf() {
        goTo(m_app->exportPage);
        m_app->exportPage->showPdfReport();
        QApplication::processEvents();
        auto* page = m_app->reportPage;
        QTRY_VERIFY_WITH_TIMEOUT(page->sectionCount() >= 6, 2000);
        QVERIFY(page->html().contains("PHY104 Laboratory Report"));
        QVERIFY2(page->html().contains("img://diagram-"), "the diagrams were not captured");
        QVERIFY(page->html().contains("img://chart-"));
        QVERIFY(page->html().contains("Thin-Film Interference"));

        // a section that is unticked leaves the document immediately
        QCheckBox* diagrams = nullptr;
        for (auto* c : page->findChildren<QCheckBox*>())
            if (c->text().startsWith("Diagrams")) diagrams = c;
        QVERIFY2(diagrams, "no diagrams checkbox");
        click(diagrams);
        QVERIFY(!page->html().contains("img://diagram-"));
        click(diagrams);
        QVERIFY(page->html().contains("img://diagram-"));

        // the file really is a PDF, and the recent-export card remembers it
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/PHY104";
        QDir(dir).removeRecursively();
        click(page->saveButton());
        const QString path = dir + "/PHY104_report_" + QDate::currentDate().toString("yyyy-MM-dd") + ".pdf";
        QVERIFY2(QFile::exists(path), qPrintable("no PDF at " + path));
        QVERIFY(QFileInfo(path).size() > 2000);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.read(5), QByteArray("%PDF-"));
        file.close();
        QCOMPARE(QString::fromStdString(m_app->context().log.lastExport().format), QString("PDF report"));
        QVERIFY(allLabelTexts(page).contains(QFileInfo(path).fileName()));
    }
};

QTEST_MAIN(InteractionTest)
#include "test_interaction.moc"
