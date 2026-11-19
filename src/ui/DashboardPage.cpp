#include "DashboardPage.h"
#include "AppContext.h"
#include "Components.h"
#include "ExperimentReports.h"
#include "Icons.h"
#include "OpticsCanvases.h"
#include "Plots.h"
#include "ThemeManager.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QToolButton>
#include <QMenu>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <set>

namespace phy104 {
namespace {

/// A card that behaves like a button.
class ClickableCard : public QFrame {
    Q_OBJECT
public:
    explicit ClickableCard(QWidget* parent = nullptr) : QFrame(parent) {
        setObjectName("cardHot");
        setCursor(Qt::PointingHandCursor);
    }
signals:
    void clicked();
protected:
    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && rect().contains(e->pos())) emit clicked();
    }
};

struct CardInfo {
    const char* kicker; const char* title; const char* text;
    PreviewIcon::Kind kind; const char* experiment; int page;
};

const CardInfo kCards[4] = {
    {"01 / GEOMETRICAL OPTICS", "Refraction & TIR", "Snell's law, critical angle and total internal reflection.", PreviewIcon::Refraction, "Refraction & TIR", 1},
    {"02 / IMAGE FORMATION", "Lenses & Mirrors", "Thin lenses, spherical mirrors and principal-ray tracing.", PreviewIcon::Lens, "Lenses & Mirrors", 2},
    {"03 / MINIMUM DEVIATION", "Prisms", "Trace an optical path and determine the refractive index.", PreviewIcon::Prism, "Prisms", 3},
    {"04 / WAVE OPTICS", "Wave Interference (Double-Slit)", "Young's experiment, fringe spacing and intensity profiles.", PreviewIcon::Waves, "Wave Interference", 4}};

struct ExperimentLink { const char* title; const char* description; const char* icon; int page; };
const ExperimentLink kAllExperiments[11] = {
    {"Refraction & TIR", "Snell's law, critical angle and total internal reflection.", "split", 1},
    {"Lenses & Mirrors", "Thin lenses, mirrors and principal-ray imaging.", "focus", 2},
    {"Prisms", "Minimum deviation, index and dispersion.", "triangle", 3},
    {"Wave Interference (Double-Slit)", "Young's double slit, fringe spacing and intensity.", "waves", 4},
    {"Polarization (Malus & Brewster)", "Malus' law, Fresnel reflection and Brewster angle.", "contrast", 5},
    {"Single-Slit Diffraction", "First minimum, screen geometry and two-slit envelope.", "slit", 6},
    {"Diffraction Grating", "Orders, resolving power and free spectral range.", "align-justify", 7},
    {"Thin-Film Interference", "Reflection, transmission and visible film colours.", "layers", 8},
    {"Michelson Interferometry", "Fringes, mirror motion and wavelength.", "git-merge", 9},
    {"Newton's Rings", "Ring radii, contact and wavelength.", "circle-dot", 10},
    {"Thick Lens & Aberration", "Principal planes, focal lengths and spherical aberration.", "aperture", 11}};

QWidget* metricCard(const QString& value, const QString& title, const QString& text, QLabel** valueOut = nullptr) {
    auto* card = new QFrame;
    card->setObjectName("card");
    auto* l = new QHBoxLayout(card);
    l->setContentsMargins(16, 14, 16, 14);
    l->setSpacing(14);
    auto* v = new QLabel(value);
    v->setObjectName("metricValue");
    auto* box = new QVBoxLayout;
    box->setSpacing(2);
    auto* t = new QLabel(title);
    t->setObjectName("metricTitle");
    auto* d = new QLabel(text);
    d->setObjectName("metricText");
    box->addStretch();
    box->addWidget(t);
    box->addWidget(d);
    box->addStretch();
    l->addWidget(v);
    l->addLayout(box, 1);
    if (valueOut) *valueOut = v;
    return card;
}

/// One row of the activity log sidebar: run number, experiment, result, status.
class LogRow : public QFrame {
    Q_OBJECT
public:
    explicit LogRow(QWidget* parent = nullptr) : QFrame(parent) {
        setObjectName("logRow");
        setCursor(Qt::PointingHandCursor);
        auto* l = new QHBoxLayout(this);
        l->setContentsMargins(10, 6, 10, 6);
        l->setSpacing(8);

        m_run = new QLabel;
        m_run->setObjectName("logRun");
        m_run->setMinimumWidth(34);
        m_run->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

        m_experiment = new QLabel;
        m_experiment->setObjectName("logExperiment");
        m_experiment->setWordWrap(true);

        m_result = new QLabel;
        m_result->setObjectName("logResult");
        m_result->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_result->setMinimumWidth(70);

        m_status = new QLabel("●");
        m_status->setObjectName("logStatus");
        m_status->setMinimumWidth(16);

        l->addWidget(m_run);
        l->addWidget(m_experiment, 1);
        l->addWidget(m_result);
        l->addWidget(m_status);
    }

    void setRun(const QString& text) { m_run->setText(text); }
    void setExperiment(const QString& text) { m_experiment->setText(text); }
    void setResult(const QString& text) { m_result->setText(text); }
    void setValid(bool valid) {
        QPalette pal = m_status->palette();
        pal.setColor(QPalette::WindowText, valid ? ThemeManager::instance().theme().lime : ThemeManager::instance().theme().error);
        m_status->setPalette(pal);
    }
    QString runId() const { return m_runId; }
    void setRunId(const QString& id) { m_runId = id; }

private:
    QLabel* m_run;
    QLabel* m_experiment;
    QLabel* m_result;
    QLabel* m_status;
    QString m_runId;
};

} // namespace

DashboardPage::DashboardPage(AppContext* ctx, QWidget* parent)
    : BasePage("Dashboard", "Your optical bench / choose an experiment or resume a saved run", ctx, parent) {

    // ===================================================================== three-column layout
    //   left (fixed): activity log sidebar
    //   center (flex): featured experiments + browse all
    //   right (fixed): lab notebook + recent export
    auto cols = addThreeColumns(260, 260);

    // ===================================================================== left: activity log sidebar
    auto* logPanel = new Panel("ACTIVITY LOG", std::string("SESSION ") + QString::number(ctx->log.sessionNumber()).rightJustified(2, '0'));
    logPanel->body()->setSpacing(8);
    logPanel->body()->setContentsMargins(0, 0, 0, 0);

    auto* logHint = new QLabel("Recent calculations — tap a row to reopen");
    logHint->setObjectName("caption");
    logHint->setWordWrap(true);
    logPanel->body()->addWidget(logHint);

    // scrollable log list
    m_logScroll = new QScrollArea;
    m_logScroll->setWidgetResizable(true);
    m_logScroll->setFrameShape(QFrame::NoFrame);
    m_logScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_logList = new QWidget;
    m_logLayout = new QVBoxLayout(m_logList);
    m_logLayout->setSpacing(2);
    m_logLayout->setContentsMargins(0, 0, 0, 0);
    m_logLayout->addStretch();
    m_logScroll->setWidget(m_logList);
    logPanel->body()->addWidget(m_logScroll, 1);

    auto* logButtons = new QHBoxLayout;
    logButtons->setSpacing(6);
    m_deleteRun = new QPushButton("Delete selected");
    m_deleteRun->setObjectName("secondary");
    m_deleteRun->setEnabled(false);
    m_clearLog = new QPushButton("Clear all");
    m_clearLog->setObjectName("flat");
    logButtons->addWidget(m_deleteRun);
    logButtons->addWidget(m_clearLog);
    logPanel->body()->addLayout(logButtons);

    cols.left->addWidget(logPanel, 1);

    // ===================================================================== center: experiments
    // --- metrics
    auto* metrics = new QHBoxLayout;
    metrics->setSpacing(12);
    QLabel* experimentCount = nullptr;
    metrics->addWidget(metricCard("11", "EXPERIMENTS", "Geometrical & wave optics", &experimentCount));
    experimentCount->setObjectName("experimentCount");
    metrics->addWidget(metricCard("00", "SAVED CALCULATIONS", "Current lab session", &m_savedValue));
    metrics->addWidget(metricCard("00", "DATA SERIES", "Ready for Excel / CSV", &m_seriesValue));
    cols.center->addLayout(metrics);

    // --- browse all experiments
    auto* browseRow = new QHBoxLayout;
    browseRow->addStretch();
    m_browseExperiments = new QToolButton;
    m_browseExperiments->setObjectName("browseExperiments");
    m_browseExperiments->setText("Browse all 11");
    m_browseExperiments->setPopupMode(QToolButton::InstantPopup);
    m_browseExperiments->setToolTip("Open any of the 11 experiment screens");
    auto* experimentMenu = new QMenu(m_browseExperiments);
    experimentMenu->setObjectName("allExperimentsMenu");
    for (int i = 0; i < 11; ++i) {
        const ExperimentLink& experiment = kAllExperiments[i];
        const Theme& theme = ThemeManager::instance().theme();
        QAction* action = experimentMenu->addAction(
            Icons::get(experiment.icon, theme.text, theme.amber, 18),
            QString("%1 · %2").arg(i + 1, 2, 10, QChar('0')).arg(QString::fromUtf8(experiment.title)));
        action->setToolTip(QString::fromUtf8(experiment.description));
        const int page = experiment.page;
        connect(action, &QAction::triggered, this, [this, page] { emit openPageRequested(page); });
    }
    m_browseExperiments->setMenu(experimentMenu);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [experimentMenu] {
        const Theme& theme = ThemeManager::instance().theme();
        const auto actions = experimentMenu->actions();
        for (int i = 0; i < 11; ++i) {
            const ExperimentLink& experiment = kAllExperiments[i];
            actions[i]->setIcon(Icons::get(experiment.icon, theme.text, theme.amber, 18));
        }
    });
    browseRow->addWidget(m_browseExperiments);
    cols.center->addLayout(browseRow);

    // --- four featured experiment cards
    auto* grid = new QGridLayout;
    grid->setSpacing(12);
    for (int i = 0; i < 4; ++i) {
        const CardInfo& info = kCards[i];
        auto* card = new ClickableCard;
        auto* l = new QHBoxLayout(card);
        l->setContentsMargins(15, 15, 15, 15);
        l->setSpacing(15);
        l->addWidget(new PreviewIcon(info.kind), 0, Qt::AlignTop);
        auto* text = new QVBoxLayout;
        text->setSpacing(6);
        auto* kicker = new QLabel(info.kicker);
        kicker->setObjectName("cardKicker");
        auto* title = new QLabel(info.title);
        title->setObjectName("cardTitle");
        title->setWordWrap(true);
        auto* desc = new QLabel(info.text);
        desc->setObjectName("cardText");
        desc->setWordWrap(true);
        auto* result = new QLabel("—");
        result->setObjectName("cardResult");
        m_results << result;
        text->addWidget(kicker);
        text->addWidget(title);
        text->addWidget(desc);
        text->addWidget(result);
        text->addStretch();
        auto* arrow = new QLabel;
        arrow->setPixmap(Icons::pixmap("arrow-up-right", ThemeManager::instance().theme().amber, 14));
        l->addLayout(text, 1);
        auto* arrowCol = new QVBoxLayout;
        arrowCol->addStretch();
        arrowCol->addWidget(arrow);
        l->addLayout(arrowCol);
        const int page = info.page;
        connect(card, &ClickableCard::clicked, this, [this, page] { emit openPageRequested(page); });
        grid->addWidget(card, i / 2, i % 2);
    }
    cols.center->addLayout(grid, 1);

    // ===================================================================== right: lab notebook + export
    auto* notebook = new Panel("LAB NOTEBOOK", "AUTOSAVED");
    notebook->body()->setSpacing(10);
    notebook->body()->setContentsMargins(0, 0, 0, 0);
    auto* blurb = new QLabel("Review formula references, record observations and export your measured data.");
    blurb->setObjectName("cardText");
    blurb->setWordWrap(true);
    auto* openNotes = new QPushButton("Open notes");
    openNotes->setObjectName("secondary");
    auto* exportData = new QPushButton("Export data");
    exportData->setObjectName("primary");
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(openNotes);
    buttons->addWidget(exportData);
    m_lastSaved = new QLabel;
    m_lastSaved->setObjectName("hint");
    notebook->body()->addWidget(blurb);
    notebook->body()->addLayout(buttons);
    notebook->body()->addWidget(m_lastSaved);
    notebook->body()->addStretch();
    cols.right->addWidget(notebook, 1);

    // ===================================================================== right: run comparison chart
    auto* comparisonPanel = new Panel("RUN COMPARISON", "OVERLAY MULTIPLE RUNS");
    comparisonPanel->body()->setSpacing(8);
    comparisonPanel->body()->setContentsMargins(0, 0, 0, 0);

    auto* filterHint = new QLabel("Experiment");
    filterHint->setObjectName("caption");
    filterHint->setWordWrap(true);
    comparisonPanel->body()->addWidget(filterHint);

    m_comparisonFilter = new QComboBox;
    m_comparisonFilter->addItem("All experiments", "");
    comparisonPanel->body()->addWidget(m_comparisonFilter);

    auto* listHint = new QLabel("Select runs to overlay");
    listHint->setObjectName("caption");
    listHint->setWordWrap(true);
    comparisonPanel->body()->addWidget(listHint);

    m_runScroll = new QScrollArea;
    m_runScroll->setWidgetResizable(true);
    m_runScroll->setFrameShape(QFrame::NoFrame);
    m_runScroll->setMaximumHeight(150);
    m_runScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_runListContainer = new QWidget;
    m_runListLayout = new QVBoxLayout(m_runListContainer);
    m_runListLayout->setSpacing(2);
    m_runListLayout->setContentsMargins(0, 0, 0, 0);
    m_runListLayout->addStretch();
    m_runScroll->setWidget(m_runListContainer);
    comparisonPanel->body()->addWidget(m_runScroll, 1);

    m_comparisonChart = new LineChart;
    m_comparisonChart->setMinimumHeight(180);
    comparisonPanel->body()->addWidget(m_comparisonChart, 1);

    cols.right->addWidget(comparisonPanel, 1);

    // ===================================================================== connections
    connect(m_comparisonFilter, &QComboBox::currentIndexChanged, this, &DashboardPage::refreshComparison);
    connect(openNotes, &QPushButton::clicked, this, [this] { emit openPageRequested(m_notesPage); });
    connect(exportData, &QPushButton::clicked, this, [this] { emit openPageRequested(m_exportPage); });
    connect(m_deleteRun, &QPushButton::clicked, this, &DashboardPage::deleteSelectedRun);
    connect(m_clearLog, &QPushButton::clicked, this, [this] {
        // delete from newest to oldest so indices stay valid
        auto runs = m_ctx->log.runs();
        std::sort(runs.begin(), runs.end(), [](const RunRecord& a, const RunRecord& b) {
            return a.id > b.id;
        });
        for (const auto& r : runs)
            m_ctx->deleteRun(r.id);
        m_selectedRunId = -1;
        refresh();
    });
    connect(ctx, &AppContext::snapshotChanged, this, &DashboardPage::refresh);
    connect(ctx, &AppContext::snapshotChanged, this, &DashboardPage::refreshComparison);
    connect(ctx, &AppContext::runsChanged, this, &DashboardPage::refresh);
    connect(ctx, &AppContext::runsChanged, this, &DashboardPage::refreshComparison);
    connect(ctx, &AppContext::notesSaved, this, &DashboardPage::refresh);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &DashboardPage::refresh);
    refresh();
    refreshComparison();
}

// ======================================================================== activity log
void DashboardPage::refreshLog() {
    // clear existing rows (keep the stretch)
    while (m_logLayout->count() > 1) {
        auto* item = m_logLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    const auto recent = m_ctx->log.recent(10);
    m_selectedRunId = -1;

    if (recent.empty()) {
        auto* empty = new QLabel("No activity yet — press Calculate & Simulate");
        empty->setObjectName("hint");
        empty->setAlignment(Qt::AlignHCenter);
        empty->setWordWrap(true);
        m_logLayout->insertWidget(m_logLayout->count() - 1, empty);
        m_deleteRun->setEnabled(false);
        return;
    }

    for (const RunRecord& r : recent) {
        auto* row = new LogRow;
        const QString label = QString::fromStdString(SessionLog::runLabel(r.id));
        row->setRun(label);
        row->setRunId(label);
        row->setExperiment(QString::fromStdString(r.description));
        row->setResult(QString::fromStdString(r.result));
        row->setValid(r.valid);

        row->setProperty("runId", r.id);
        // clicks are handled by the event filter installed on the DashboardPage
        row->installEventFilter(this);

        m_logLayout->insertWidget(m_logLayout->count() - 1, row);
    }

    m_deleteRun->setEnabled(m_selectedRunId >= 0);
}

bool DashboardPage::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (auto* row = qobject_cast<LogRow*>(obj)) {
            const int id = row->property("runId").toInt();
            m_selectedRunId = id;
            m_deleteRun->setEnabled(id >= 0);
            // highlight selected row
            for (int i = 0; i < m_logLayout->count() - 1; ++i) {
                if (auto* w = m_logLayout->itemAt(i)->widget()) {
                    w->setProperty("selected", w == row);
                    w->style()->unpolish(w);
                    w->style()->polish(w);
                    w->update();
                }
            }
            // reopen the experiment page by matching the run's experiment name
            for (const RunRecord& r : m_ctx->log.runs()) {
                if (r.id == id) {
                    // find the page index for this experiment
                    const int page = pageForExperiment(r.experiment);
                    if (page >= 0) emit openPageRequested(page);
                    break;
                }
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

int DashboardPage::pageForExperiment(const std::string& name) const {
    static const struct { const char* name; int page; } map[] = {
        {"Refraction & TIR", 1}, {"Lenses & Mirrors", 2}, {"Prisms", 3},
        {"Wave Interference", 4}, {"Polarization", 5}, {"Single-Slit Diffraction", 6},
        {"Diffraction Grating", 7}, {"Thin-Film Interference", 8}, {"Michelson Interferometry", 9},
        {"Newton's Rings", 10}, {"Thick Lens & Aberration", 11}};
    for (const auto& m : map)
        if (m.name == name) return m.page;
    return -1;
}

// ======================================================================== refresh (metrics + cards)
void DashboardPage::refresh() {
    m_savedValue->setText(QString::number(m_ctx->log.savedCalculations()).rightJustified(2, '0'));

    int series = 0;
    for (int i = 0; i < 4; ++i) {
        const Snapshot* snap = m_ctx->log.snapshot(kCards[i].experiment);
        m_results[i]->setText(snap ? QString::fromStdString(snap->headline) : "—");
        if (snap && !snap->series.rows.empty()) ++series;
    }
    m_seriesValue->setText(QString::number(series).rightJustified(2, '0'));

    refreshLog();

    const QString when = m_ctx->lastSavedTime();
    m_lastSaved->setText((when == "restored" ? QString("Session restored") : std::string("Last saved ") + when) + std::string(" · ") + m_ctx->sessionFile);
}

void DashboardPage::deleteSelectedRun() {
    if (m_selectedRunId <= 0) return;
    if (m_ctx->deleteRun(m_selectedRunId)) {
        m_selectedRunId = -1;
        refresh();
    }
}

// ======================================================================== run comparison chart
ChartSeries DashboardPage::seriesToChart(const DataSeries& s, const QColor& color) const {
    ChartSeries cs;
    cs.color = color;
    cs.line = true;
    cs.markers = true;
    if (s.headers.size() < 2 || s.rows.empty()) return cs;

    // Plot column 1 (Y) against column 0 (X), skipping NaN and non-finite rows.
    for (const auto& row : s.rows) {
        if (row.size() < 2) continue;
        double x = row[0], y = row[1];
        if (!std::isfinite(x) || !std::isfinite(y)) continue;
        cs.points.append(QPointF(x, y));
    }
    // Sort by X for clean line drawing.
    std::sort(cs.points.begin(), cs.points.end(),
              [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });
    return cs;
}

void DashboardPage::refreshComparison() {
    // Clear existing checkboxes.
    qDeleteAll(m_runCheckboxes);
    m_runCheckboxes.clear();

    // Clear existing run list widgets (keep the stretch at the end).
    while (m_runListLayout->count() > 1) {
        auto* item = m_runListLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    const auto runs = m_ctx->log.runs();
    const QString filter = m_comparisonFilter->currentData().toString();

    // Rebuild the experiment filter dropdown (keep "All experiments").
    m_comparisonFilter->blockSignals(true);
    while (m_comparisonFilter->count() > 1) m_comparisonFilter->removeItem(1);
    std::set<std::string> experiments;
    for (const auto& r : runs) experiments.insert(r.experiment);
    for (const auto& e : experiments)
        m_comparisonFilter->addItem(QString::fromUtf8(e.c_str()), QString::fromUtf8(e.c_str()));
    m_comparisonFilter->blockSignals(false);

    // Filter runs.
    std::vector<const RunRecord*> filtered;
    for (const auto& r : runs) {
        if (!filter.isEmpty() && QString::fromUtf8(r.experiment.c_str()) != filter) continue;
        // Only show runs that have a data series with at least 2 columns and rows.
        if (r.series.headers.size() < 2 || r.series.rows.empty()) continue;
        filtered.push_back(&r);
    }
    // Newest first.
    std::sort(filtered.begin(), filtered.end(),
              [](const RunRecord* a, const RunRecord* b) { return a->id > b->id; });

    const Theme& t = ThemeManager::instance().theme();
    const QColor palette[] = {t.amber, t.lime, t.blue, t.sage, t.muted, t.text, t.error};
    int colorIdx = 0;

    QList<ChartSeries> chartSeries;
    QStringList legendItems;

    for (const auto* r : filtered) {
        auto* cb = new QCheckBox;
        const QString label = QString("%1 · %2")
                                  .arg(QString::fromStdString(SessionLog::runLabel(r->id)))
                                  .arg(QString::fromStdString(r->description));
        cb->setText(label);
        cb->setChecked(true);  // default: show all
        cb->setProperty("runId", r->id);
        connect(cb, &QCheckBox::toggled, this, &DashboardPage::refreshComparison);
        m_runListLayout->insertWidget(m_runListLayout->count() - 1, cb);
        m_runCheckboxes.insert(r->id, cb);

        if (!cb->isChecked()) continue;

        QColor color = palette[colorIdx % (sizeof(palette) / sizeof(palette[0]))];
        ++colorIdx;

        ChartSeries cs = seriesToChart(r->series, color);
        if (!cs.points.empty()) {
            chartSeries.append(cs);
            legendItems.append(QString::fromUtf8(SessionLog::runLabel(r->id)) + " · " +
                               QString::fromUtf8(r->description));
        }
    }

    // Set up chart axes from the first usable series.
    if (!chartSeries.empty()) {
        double xMin = chartSeries.first().points.first().x();
        double xMax = chartSeries.first().points.first().x();
        double yMin = chartSeries.first().points.first().y();
        double yMax = chartSeries.first().points.first().y();
        for (const auto& cs : chartSeries) {
            for (const auto& pt : cs.points) {
                xMin = std::min(xMin, pt.x());
                xMax = std::max(xMax, pt.x());
                yMin = std::min(yMin, pt.y());
                yMax = std::max(yMax, pt.y());
            }
        }
        const double xPad = (xMax - xMin) * 0.05;
        const double yPad = (yMax - yMin) * 0.05;
        m_comparisonChart->setAxes(xMin - xPad, xMax + xPad, 4, 2,
                                   QString::fromUtf8(filtered.front()->series.headers[0]),
                                   yMin - yPad, yMax + yPad, 4, 2,
                                   QString::fromUtf8(filtered.front()->series.headers[1]));
    } else {
        m_comparisonChart->setAxes(0, 1, 4, 2, "X", 0, 1, 4, 2, "Y");
    }

    m_comparisonChart->setSeries(chartSeries);
    m_comparisonChart->setLegend(legendItems, "Runs overlaid · column 1 vs column 0");
}

} // namespace phy104

#include "DashboardPage.moc"