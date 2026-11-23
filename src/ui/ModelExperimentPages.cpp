#include "ModelExperimentPages.h"
#include "AppContext.h"
#include "FieldIds.h"
#include "Components.h"
#include "OpticsCanvases.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QSignalBlocker>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>

namespace phy104 {
namespace WE = WaveExperiments;
namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }

QWidget* compactLabeled(const QString& text, QWidget* field) {
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    auto* label = new QLabel(text);
    label->setObjectName("fieldLabel");
    label->setWordWrap(true);
    label->setMinimumWidth(92);
    label->setMaximumWidth(112);
    label->setAlignment(Qt::AlignVCenter | (row->layoutDirection() == Qt::RightToLeft ? Qt::AlignRight : Qt::AlignLeft));
    layout->addWidget(label, 0);
    layout->addWidget(field, 1);
    return row;
}

QWidget* addFoldout(QVBoxLayout* parent, const QString& title, QLayout* contents) {
    auto* toggle = new QToolButton;
    toggle->setObjectName("experimentFoldout");
    toggle->setText(title);
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setCheckable(true);
    toggle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* body = new QWidget;
    body->setLayout(contents);
    body->hide();
    parent->addWidget(toggle);
    parent->addWidget(body);
    QObject::connect(toggle, &QToolButton::toggled, body, [toggle, body](bool expanded) {
        toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
        body->setVisible(expanded);
    });
    return body;
}
}

// ============================================================================ ModelPage
ModelPage::ModelPage(const QString& title, const QString& subtitle, const QString& experiment, AppContext* ctx, QWidget* parent)
    : ExperimentPage(title, subtitle, experiment, ctx, parent) {
    useSaveExportHeader();
    m_controls = new Panel("Experiment controls");
    m_controls->body()->setContentsMargins(8, 4, 8, 8);
    m_controls->body()->setSpacing(0);
    auto* controlsContent = new QWidget;
    m_fieldLayout = new QVBoxLayout(controlsContent);
    m_fieldLayout->setContentsMargins(2, 2, 4, 2);
    m_fieldLayout->setSpacing(4);
    m_controls->body()->addWidget(controlsContent, 1);

    // Slider debounce timer: coalesces rapid valueChanged calls into a single apply after 16 ms (~60 fps).
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    m_debounceTimer->setInterval(16);
    connect(m_debounceTimer, &QTimer::timeout, this, &ModelPage::debounceApply);
}

void ModelPage::debounceApply() {
    apply(false);
}

UnitField* ModelPage::addField(const QString& id, const QString& label, const QString& unit, double value, int decimals, bool signedValue, bool spinner, double step) {
    auto* f = new UnitField(unit);
    f->setShowPlus(signedValue);
    if (spinner) f->setSpinner(true, step);
    f->setValue(value, decimals);
    bind(id, f);
    m_fieldLayout->addWidget(compactLabeled(label, f));
    connect(f, &UnitField::edited, this, [this] { apply(false); });
    return f;
}

QComboBox* ModelPage::addCombo(const QString& label, const QStringList& items, const QList<double>& values) {
    auto* c = new QComboBox;
    for (int i = 0; i < items.size(); ++i) c->addItem(items[i], values.value(i, 0.0));
    m_fieldLayout->addWidget(compactLabeled(label, c));
    connect(c, &QComboBox::activated, this, [this] { apply(false); });
    return c;
}

ModeTabs* ModelPage::addTabs(const QStringList& tabs) {
    m_tabs = new ModeTabs(tabs);
    m_fieldLayout->insertWidget(0, m_tabs);
    connect(m_tabs, &ModeTabs::currentChanged, this, [this] { apply(false); });
    return m_tabs;
}

void ModelPage::build(const QString& diagramTitle, QWidget* canvas, const QString& chartTitle, const QString& hint) {
    auto cols = addThreeColumns();
    buildButtons(m_fieldLayout, hint);
    if (m_hint) {
        m_calc->setToolTip(m_hint->text());
        m_hint->hide();
    }
    cols.left->addWidget(m_controls, 1);

    m_diagram = new Panel(diagramTitle, "");
    m_diagram->body()->setContentsMargins(0, 0, 0, 0);
    m_diagram->body()->addWidget(canvas);
    m_chart = new LineChart;
    m_chartPanel = new Panel(chartTitle, "");
    m_chartPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_chartPanel->body()->addWidget(m_chart);
    cols.center->addWidget(m_diagram, 3);
    cols.center->addWidget(m_chartPanel, 2);

    m_results = new Panel(QString("Results · %1").arg(m_experiment), "");
    m_results->body()->setContentsMargins(8, 4, 8, 8);
    m_results->body()->setSpacing(0);
    auto* resultsContent = new QWidget;
    auto* resultsLayout = new QVBoxLayout(resultsContent);
    resultsLayout->setContentsMargins(2, 2, 4, 2);
    resultsLayout->setSpacing(6);
    m_resultCard1 = new ResultCard;
    m_resultCard2 = new ResultCard;
    m_resultCard1->hide();
    m_resultCard2->hide();
    m_reg1 = new RegisterView;
    m_reg2 = new RegisterView;
    m_interpretation = new NoticeCard;
    m_guard = new NoticeCard;
    resultsLayout->addWidget(m_resultCard1);
    resultsLayout->addWidget(m_resultCard2);
    resultsLayout->addWidget(m_reg1);
    resultsLayout->addWidget(m_reg2);
    resultsLayout->addWidget(m_interpretation);
    resultsLayout->addWidget(m_guard);
    resultsLayout->addStretch();
    m_results->body()->addWidget(resultsContent, 1);
    cols.right->addWidget(m_results, 1);
    cols.right->addStretch();
    setPanels(m_diagram, m_chartPanel, m_results, "", "");
}

void ModelPage::useNativeResultCards() {
    m_useNativeResultCards = true;
    m_reg1->hide();
    m_reg2->hide();
    m_resultCard1->show();
    m_resultCard2->show();
}

void ModelPage::retitle(const QString& diagramTitle, const QString& chartTitle) {
    m_diagramTitle = diagramTitle;
    if (m_diagram) m_diagram->setTitle(diagramTitle);
    if (m_chartPanel) m_chartPanel->setTitle(chartTitle);
}

void ModelPage::showChart(const ChartData& c) {
    ChartSeries curve, curve2, marker;
    curve.line = true;
    for (const auto& [x, y] : c.curve) curve.points << QPointF(x, y);
    if (!c.curve2.empty()) {
        curve2.line = true;
        for (const auto& [x, y] : c.curve2) curve2.points << QPointF(x, y);
    }
    marker.markers = true;
    marker.color = QColor();                                            // theme sage
    for (const auto& [x, y] : c.marker) marker.points << QPointF(x, y);
    m_chart->clearAnnotations();
    m_chart->setAxes(c.xMin, c.xMax, c.xTicks, c.xDecimals, q(c.xLabel), c.yMin, c.yMax, c.yTicks, c.yDecimals, q(c.yLabel));
    QList<ChartSeries> series = {curve, marker};
    if (!c.curve2.empty()) series.insert(1, curve2);                    // second curve between curve and marker
    m_chart->setSeries(series);
    QStringList legends = {q(c.legend)};
    if (!c.legend2.empty()) legends << q(c.legend2);
    m_chart->setLegend(legends, QString());
}

void ModelPage::reset() {
    resetFields();
    m_haveLast = false;
    apply(false);
}

void ModelPage::apply(bool record) {
    if (!readFields()) return;
    const ModelReport mr = compute(reportContext());
    const Report& r = mr.report;
    if (m_useNativeResultCards) {
        if (!r.registers.empty()) m_resultCard1->setRegister(r.registers[0]);
        if (r.registers.size() > 1) m_resultCard2->setRegister(r.registers[1]);
        m_resultCard1->setVisible(!r.registers.empty());
        m_resultCard2->setVisible(r.registers.size() > 1);
    } else {
        showRegisters(m_reg1, m_reg2, r);
    }
    const Notice& firstNotice = r.notices.empty() ? Notice{"", "", false} : r.notices[0];
    const Notice& secondNotice = r.notices.size() > 1 ? r.notices[1] : firstNotice;
    if (r.paused) {                                                    // the red "paused" card replaces the interpretation
        m_interpretation->hide();
        m_guard->setNotice(firstNotice);
    } else {
        m_interpretation->show();
        m_interpretation->setNotice(firstNotice);
        m_guard->setNotice(secondNotice);
    }
    applyIssues(r);
    if (r.paused) return;

    m_last = r;
    m_haveLast = true;
    captureValid();
    updateCanvas();
    showChart(mr.chart);

    DataSeries series;
    series.id = m_experiment.toLower().replace("'", "").section(' ', 0, 0).toStdString() + "_sweep";
    series.name = m_experiment.toStdString() + " curve";
    series.description = r.headline;
    series.headers = {mr.chart.xLabel, mr.chart.yLabel};
    const std::size_t step = std::max<std::size_t>(1, mr.chart.curve.size() / 20);
    for (std::size_t i = 0; i < mr.chart.curve.size(); i += step) series.rows.push_back({mr.chart.curve[i].first, mr.chart.curve[i].second});
    m_ctx->publish({m_experiment.toStdString(), r.parameters, r.registers, r.headline, series, {}});
    if (record) {
        m_ctx->recordRun(m_experiment, runDescription(), q(r.headline), true);
        refreshMetas(false);
    }
}

// ============================================================================ Newton's rings
NewtonRingsPage::NewtonRingsPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Newton's Rings", "Dark rings in a thin air film, reflected or transmitted", "Newton's Rings", ctx, parent) {
    addTabs({"Reflected", "Dark rings"});
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Vacuum wavelength · λ", "nm", 600, 2);
    m_lambda->setToolTip("Wavelength of light in nm. Ring radius r ≈ sqrt(m·λ·R/n).");
    m_radius = addField(QString::fromLatin1(FieldIds::radiusOfCurvature), "Radius of curvature · R", "m", 1.0, 4);
    m_radius->setToolTip("Radius of curvature of the lens surface in m. Larger R = smaller rings.");
    m_order = addField(QString::fromLatin1(FieldIds::order), "Selected dark-ring order · m", "integer", 4, 0, false, true, 1);
    m_order->setToolTip("Dark ring order m. Central spot is m=0 (dark in reflected, bright in transmitted).");
    m_sigmaLambda = addField(QString::fromLatin1(FieldIds::sigmaNewtonLambda), "σ(λ) uncertainty", "nm", 0.0, 4);
    m_sigmaLambda->setToolTip("Standard uncertainty (1σ) on wavelength. Enter 0 to treat as exact.");
    m_sigmaRadius = addField(QString::fromLatin1(FieldIds::sigmaNewtonRadius), "σ(R) uncertainty", "m", 0.0, 4);
    m_sigmaRadius->setToolTip("Standard uncertainty (1σ) on radius of curvature. Enter 0 to treat as exact.");
    m_sigmaN = addField(QString::fromLatin1(FieldIds::sigmaNewtonIndex), "σ(n) uncertainty", "", 0.0, 4);
    m_sigmaN->setToolTip("Standard uncertainty (1σ) on film index. Enter 0 to treat as exact.");
    m_medium = addCombo("Film medium", {"Air · n = 1.0000", "Water · n = 1.3330", "Oil · n = 1.4000"}, {1.0, 1.333, 1.4});
    m_medium->setToolTip("Film medium between lens and plate (affects effective wavelength).");
    m_viewing = addCombo("Viewing mode", {"Reflected light", "Transmitted light"}, {0, 1});
    m_viewing->setToolTip("Reflected: dark contact center. Transmitted: bright contact center, dark rings at half orders.");
    // ---- Preset scenarios (Item 19) ----
    auto* presetGrid = new QGridLayout;
    presetGrid->setHorizontalSpacing(6);
    presetGrid->setVerticalSpacing(6);
    presetGrid->setColumnStretch(0, 1);
    presetGrid->setColumnStretch(1, 1);
    int presetIndex = 0;
    auto makePreset = [&](const QString& label, double lambda, double radius, int order, int mediumIdx, int viewingIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setToolTip(label);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(b, &QPushButton::clicked, this, [this, lambda, radius, order, mediumIdx, viewingIdx] {
            m_syncing = true;
            m_lambda->setValue(lambda, 2);
            m_radius->setValue(radius, 4);
            m_order->setValue(order, 0);
            m_medium->setCurrentIndex(mediumIdx);
            m_viewing->setCurrentIndex(viewingIdx);
            m_syncing = false;
            apply(false);
        });
        presetGrid->addWidget(b, presetIndex / 2, presetIndex % 2);
        ++presetIndex;
        return b;
    };
    m_presetAir = makePreset("Air · m=4", 600.0, 1.0, 4, 0, 0);
    m_presetWater = makePreset("Water · m=4", 600.0, 1.0, 4, 1, 0);
    m_presetOil = makePreset("Oil · m=4", 600.0, 1.0, 4, 2, 0);
    m_presetTransmitted = makePreset("Transmitted · m=4", 600.0, 1.0, 4, 0, 1);
    addFoldout(m_fieldLayout, "Presets", presetGrid);
    m_canvas = new NewtonCanvas;
    build("Lens on plate / concentric reflected rings", m_canvas, "Dark-ring radius squared vs order", "Reflected: m = 0 is the dark contact center. Transmitted: it is bright and the dark rings sit at half orders. t ≈ r²/(2R).");
    auto* newtonHelpBtn = new QToolButton;
    newtonHelpBtn->setText("?");
    newtonHelpBtn->setFixedSize(24, 24);
    newtonHelpBtn->setToolTip("How to use — Newton's Rings");
    newtonHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(newtonHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Newton's Rings",
            "<h3>Newton's Rings</h3>"
            "<p><b>λ:</b> wavelength of light in nm. Ring radius satisfies r² ≈ m·λ·R/n.</p>"
            "<p><b>R:</b> radius of curvature of the lens surface in m.</p>"
            "<p><b>m:</b> dark ring order (m = 0 at the contact center).</p>"
            "<p><b>Medium:</b> the film between lens and plate changes the effective wavelength.</p>"
            "<p><b>Viewing:</b> reflected = dark center; transmitted = bright center.</p>"
            "<p>Presets for air/water/oil media and reflected/transmitted viewing.</p>");
    });
    auto* newtonHelpBar = new QHBoxLayout;
    newtonHelpBar->setContentsMargins(0, 0, 0, 0);
    newtonHelpBar->addStretch(1);
    newtonHelpBar->addWidget(newtonHelpBtn);
    m_diagram->body()->addLayout(newtonHelpBar);
    reset();
}
ModelReport NewtonRingsPage::compute(const ReportContext& ctx) {
    return WE::newtonRings({m_lambda->value(), m_radius->value(), m_order->value(), m_medium->currentData().toDouble(),
                            m_sigmaLambda->value(), m_sigmaRadius->value(), m_sigmaN->value(),
                            m_viewing->currentData().toDouble() > 0.5}, ctx);
}
void NewtonRingsPage::updateCanvas() {
    const bool transmitted = m_viewing->currentData().toDouble() > 0.5;
    m_canvas->setInput({m_lambda->value(), m_radius->value(), m_order->value(), m_medium->currentData().toDouble(), static_cast<double>(transmitted)});
    retitle(transmitted ? "Lens on plate / transmitted rings" : "Lens on plate / reflected rings",
            transmitted ? "Transmitted dark-ring radius squared vs half order" : "Dark-ring radius squared vs order");
}
void NewtonRingsPage::resetFields() { m_lambda->setValue(600, 2); m_radius->setValue(1.0, 4); m_order->setValue(4, 0); m_sigmaLambda->setValue(0.0, 4); m_sigmaRadius->setValue(0.0, 4); m_sigmaN->setValue(0.0, 4); m_medium->setCurrentIndex(0); m_viewing->setCurrentIndex(0); m_tabs->setCurrent(0); }
QString NewtonRingsPage::runDescription() const {
    return QString("m = %1 · λ %2 nm · R %3 m · %4").arg(m_order->text(), m_lambda->text(), m_radius->text(),
                                                         m_viewing->currentData().toDouble() > 0.5 ? "transmitted" : "reflected");
}

// ============================================================================ Michelson
MichelsonPage::MichelsonPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Michelson Interferometer — Wavelength of Laser Beam", "Move M1 and count central fringes to determine λ", "Michelson Interferometry", ctx, parent) {
    addTabs({"Displacement", "Fringes", "Laser wavelength"});
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Laser wavelength · λ", "nm", 632.8, 2);
    m_lambda->setToolTip("Laser wavelength in nm. In Laser wavelength mode select whether λ, N, or d is the unknown.");

    m_source = new QComboBox;
    for (const auto& s : ctx->presets.sources) m_source->addItem(QString::fromStdString(s.name), s.wavelengthNm);
    m_source->addItem("Custom", -1.0);
    m_fieldLayout->addWidget(compactLabeled("Source preset", m_source));
    m_source->setToolTip("Choose a standard light source preset, or Custom to enter λ manually.");

    m_dx = addField(QString::fromLatin1(FieldIds::mirrorDisplacement), "Movable mirror shift · d / Δx", "µm", 0.75, 4, true, true, 0.05);
    m_dx->setToolTip("Laser wavelength mode: M1 displacement d in µm. Legacy modes: M2 travel Δx.");

    m_dxSlider = new QSlider(Qt::Horizontal);
    m_dxSlider->setObjectName("michelsonMirrorSlider");
    const int sliderLimit = static_cast<int>(std::lround(MichelsonCanvas::kMaxInteractiveTravelUm * 10000.0));
    m_dxSlider->setRange(-sliderLimit, sliderLimit);                  // 0.1 nm steps, ±2 µm stage travel
    m_dxSlider->setSingleStep(10);                                    // 1 nm keyboard/page adjustment
    m_dxSlider->setPageStep(1000);
    m_dxSlider->setTickPosition(QSlider::TicksBelow);
    m_dxSlider->setTickInterval(5000);
    m_dxSlider->setAccessibleName("Movable mirror displacement in micrometres");
    m_dxSlider->setToolTip("Laser wavelength mode moves M1 vertically from 0 to 2 µm; legacy modes move M2 horizontally.");
    m_fieldLayout->addWidget(compactLabeled("Mirror travel · d/Δx", m_dxSlider));

    m_fringeCount = addField(QString::fromLatin1(FieldIds::michelsonFringeCount), "Counted central fringes · N", "fringes", 2.3704, 4);
    m_fringeCount->setToolTip("Count the central fringes that pass while M1 moves. λ = 2d/N in air.");
    m_solveFor = addCombo("Solve for", {"Laser wavelength λ", "Fringe count N", "M1 displacement d"}, {0.0, 1.0, 2.0});
    m_solveFor->setToolTip("Calculate λ = 2d/N, N = 2d/λ, or d = Nλ/2.");

    m_medium = new QComboBox;
    for (const auto& m : ctx->presets.media) m_medium->addItem(QString::fromStdString(m.name), m.n);
    m_medium->addItem("Custom · n = slider", -1.0);
    m_fieldLayout->addWidget(compactLabeled("Arm medium · n", m_medium));
    m_medium->setToolTip("Medium filling the M2 arm. ΔOPD = 2n·Δx scales with n.");

    m_nSlider = new QSlider(Qt::Horizontal);
    m_nSlider->setObjectName("michelsonIndexSlider");
    m_nSlider->setRange(1000, 2500);           // 1.000 to 2.500 in 0.001 steps
    m_nSlider->setSingleStep(1);              // 0.001 keyboard/page adjustment
    m_nSlider->setPageStep(100);
    m_nSlider->setTickPosition(QSlider::TicksBelow);
    m_nSlider->setTickInterval(500);
    m_nSlider->setValue(1000);                // default: air
    m_nSlider->setAccessibleName("Refractive index of the arm medium");
    m_nSlider->setToolTip("Adjust the refractive index n of the medium in the M2 arm. ΔOPD = 2nΔx.");
    m_fieldLayout->addWidget(compactLabeled("Index n · 1.000–2.500", m_nSlider));

    m_reference = addCombo("M2 drive", {"Manual micrometer", "Piezo actuator"}, {0, 1});
    m_reference->setToolTip("Manual: move M2 by dragging/slider. Piezo: sub-nanometre actuation for fine fringes.");
    m_balance = addCombo("Beam balance", {"Equal beams · V = 1.00", "Unequal beams · V = 0.60"}, {1.0, 0.6});
    m_balance->setToolTip("Beam balance sets fringe visibility V. Equal arms give maximum contrast.");
    m_viewAngle = addField(QString::fromLatin1(FieldIds::michelsonViewAngle), "Viewing angle · θ", "°", 0.0, 2, false, true, 0.1);
    m_viewAngle->setToolTip("Viewing angle from the optical axis in Δ = 2d cos θ + λ/2.");

    // Keep less frequently used optical refinements available without pushing
    // the core experiment controls below the initial screen.
    QVBoxLayout* primaryFields = m_fieldLayout;
    auto* advancedFields = new QVBoxLayout;
    advancedFields->setContentsMargins(2, 2, 2, 2);
    advancedFields->setSpacing(4);
    addFoldout(primaryFields, "Advanced settings", advancedFields);
    m_fieldLayout = advancedFields;

    // Item 13: coherence length / source linewidth
    m_coherenceLength = addField(QString::fromLatin1(FieldIds::michelsonCoherenceLength), "Coherence length · Lc", "µm", 0.0, 2, false, true, 1);
    m_coherenceLength->setToolTip("Coherence length Lc. Fringes fade when |2·Δx| approaches Lc.");
    // Item 14: mirror tilt
    m_tiltSlider = new QSlider(Qt::Horizontal);
    m_tiltSlider->setObjectName("michelsonTiltSlider");
    m_tiltSlider->setRange(0, 1000);           // 0 to 10 mrad in 0.01 mrad steps
    m_tiltSlider->setSingleStep(10);
    m_tiltSlider->setPageStep(100);
    m_tiltSlider->setValue(0);
    m_tiltSlider->setAccessibleName("M2 mirror tilt in milliradians");
    m_tiltSlider->setToolTip("Tilt M2 to change from straight fringes (tilt = 0) to circular bullseye fringes.");
    m_fieldLayout->addWidget(compactLabeled("Mirror tilt · mrad", m_tiltSlider));
    // Item 15: glass plate in one arm
    m_plateThickness = addField(QString::fromLatin1(FieldIds::michelsonPlateThickness), "Plate thickness · t", "µm", 0.0, 2, false, true, 1);
    m_plateThickness->setToolTip("Plate thickness in µm inserted in one arm. Adds optical path (n_p−1)·t.");
    m_plateIndex = addField(QString::fromLatin1(FieldIds::michelsonPlateIndex), "Plate index · n_p", "", 1.5, 4, false, true, 0.01);
    m_plateIndex->setToolTip("Refractive index of the plate material.");
    m_fieldLayout = primaryFields;
    // ---- Preset scenarios (Item 19) ----
    auto* presetGrid = new QGridLayout;
    presetGrid->setHorizontalSpacing(6);
    presetGrid->setVerticalSpacing(6);
    presetGrid->setColumnStretch(0, 1);
    presetGrid->setColumnStretch(1, 1);
    int presetIndex = 0;
    auto makePreset = [&](const QString& label, double lambda, double dxUm, double nIdx, bool fringes) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setToolTip(label);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(32);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(b, &QPushButton::clicked, this, [this, lambda, dxUm, nIdx, fringes] {
            m_syncing = true;
            m_lambda->setValue(lambda, 2);
            m_dx->setValue(dxUm, 4);
            m_medium->setCurrentIndex(static_cast<int>(nIdx));
            m_tabs->setCurrent(fringes ? 1 : 0);
            m_syncing = false;
            apply(false);
        });
        presetGrid->addWidget(b, presetIndex / 2, presetIndex % 2);
        ++presetIndex;
        return b;
    };
    m_presetAir = makePreset("Air · Δx 0.75", 632.8, 0.75, 0, false);
    m_presetCrown = makePreset("Crown · Δx 0.75", 632.8, 0.75, 1, false);
    m_presetFlint = makePreset("Flint · Δx 0.75", 632.8, 0.75, 2, false);
    m_presetFringes = makePreset("Fringes tab", 632.8, 0.75, 0, true);
    auto* wavelengthPreset = new QPushButton("Laser · 632.8 nm · 5 fringes");
    wavelengthPreset->setObjectName("presetBtn");
    wavelengthPreset->setToolTip("Load a wavelength-measurement example: d = 1.582 µm, N = 5.");
    wavelengthPreset->setMinimumHeight(32);
    wavelengthPreset->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    presetGrid->addWidget(wavelengthPreset, presetIndex / 2, 0, 1, 2);
    connect(wavelengthPreset, &QPushButton::clicked, this, [this] {
        m_lambda->setValue(632.8, 2);
        m_dx->setValue(1.582, 4);
        m_fringeCount->setValue(5.0, 0);
        m_solveFor->setCurrentIndex(0);
        m_tabs->setCurrent(2);
        apply(false);
    });
    addFoldout(m_fieldLayout, "Presets", presetGrid);
    m_canvas = new MichelsonCanvas;
    m_canvas->setObjectName("michelsonCanvas");
    connect(m_dxSlider, &QSlider::valueChanged, this, [this](int value) {
        setMirrorDisplacement(value / 10000.0);
    });
    connect(m_dx, &UnitField::edited, this, [this] {
        bool ok = false;
        const double displacement = m_dx->value(&ok);
        if (!ok) return;
        QSignalBlocker blocker(m_dxSlider);
        m_dxSlider->setValue(static_cast<int>(std::lround(displacement * 10000.0)));
    });
    connect(m_fringeCount, &UnitField::edited, this, [this] { apply(false); });
    connect(m_viewAngle, &UnitField::edited, this, [this] { apply(false); });
    connect(m_canvas, &MichelsonCanvas::mirrorDisplacementChanged, this, &MichelsonPage::setMirrorDisplacement);

    const auto updateModeControls = [this] {
        const bool wavelengthMode = m_tabs->current() == 2;
        const int target = m_solveFor->currentData().toInt();
        const int sliderLimit = static_cast<int>(std::lround(MichelsonCanvas::kMaxInteractiveTravelUm * 10000.0));
        {
            QSignalBlocker blocker(m_dxSlider);
            m_dxSlider->setRange(wavelengthMode ? 0 : -sliderLimit, sliderLimit);
            if (wavelengthMode && m_dxSlider->value() < 0) {
                m_dxSlider->setValue(0);
                m_dx->setValue(0.0, 4);
            }
        }
        m_dx->setShowPlus(!wavelengthMode);
        bool displacementOk = false;
        const double displacement = m_dx->value(&displacementOk);
        if (displacementOk) m_dx->setValue(displacement, 4);
        m_solveFor->parentWidget()->setVisible(wavelengthMode);
        m_fringeCount->parentWidget()->setVisible(wavelengthMode);
        m_viewAngle->parentWidget()->setVisible(wavelengthMode);
        for (QWidget* legacyRow : {m_source->parentWidget(), m_medium->parentWidget(), m_nSlider->parentWidget(),
                                   m_reference->parentWidget(), m_balance->parentWidget()})
            legacyRow->setVisible(!wavelengthMode);
        m_lambda->setEnabled(!wavelengthMode || target != 0);
        m_dx->setEnabled(!wavelengthMode || target != 2);
        m_dxSlider->setEnabled(!wavelengthMode || target != 2);
        m_fringeCount->setEnabled(!wavelengthMode || target != 1);
        if (!wavelengthMode) {
            bool lambdaOk = false;
            const double lambda = m_lambda->value(&lambdaOk);
            bool presetMatch = false;
            if (lambdaOk) {
                for (int i = 0; i < m_source->count() - 1; ++i) {
                    if (std::fabs(m_source->itemData(i).toDouble() - lambda) < 1e-6) {
                        m_source->setCurrentIndex(i);
                        presetMatch = true;
                        break;
                    }
                }
            }
            if (!presetMatch) m_source->setCurrentIndex(m_source->count() - 1);
        }
        m_canvas->setMirrorMovable(!wavelengthMode || target != 2);
        m_dxSlider->setAccessibleName(wavelengthMode ? "M1 mirror displacement d in micrometres"
                                                    : "M2 mirror travel in micrometres");
        m_dxSlider->setToolTip(wavelengthMode ? "Move M1 to change d and the central fringe count."
                                             : "Drag the amber M2 mirror left or right along its arm.");
    };
    connect(m_tabs, &ModeTabs::currentChanged, this, [this, updateModeControls](int) {
        updateModeControls();
        apply(false);
    });
    connect(m_solveFor, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, updateModeControls](int) {
        updateModeControls();
        apply(false);
    });
    updateModeControls();

    // Source preset: choosing a preset sets the wavelength; editing the wavelength manually switches to "Custom".
    connect(m_source, &QComboBox::activated, this, [this] {
        const double w = m_source->currentData().toDouble();
        if (w > 0) {
            m_lambda->setValue(w, 1);
            apply(false);
        }
    });
    connect(m_lambda, &UnitField::edited, this, [this] {
        bool ok = false;
        const double v = m_lambda->value(&ok);
        if (!ok) return;
        bool matches = false;
        for (int i = 0; i < m_source->count() - 1; ++i) {
            if (std::fabs(m_source->itemData(i).toDouble() - v) < 1e-9) { matches = true; break; }
        }
        if (!matches) m_source->setCurrentIndex(m_source->count() - 1);  // "Custom"
        apply(false);
    });

    // Index slider: moving it sets n and re-applies; choosing a medium preset updates the slider.
    connect(m_nSlider, &QSlider::valueChanged, this, [this](int) {
        m_medium->setCurrentIndex(m_medium->count() - 1);  // "Custom"
        debounceApply();
    });
    connect(m_medium, &QComboBox::activated, this, [this](int index) {
        const double n = m_medium->itemData(index).toDouble();
        if (n > 0) {
            QSignalBlocker blocker(m_nSlider);
            m_nSlider->setValue(static_cast<int>(std::lround(n * 1000.0)));
        }
        apply(false);
    });

    // Item 13-15 real-time connections for new parameters
    connect(m_coherenceLength, &UnitField::edited, this, [this] { apply(false); });
    connect(m_tiltSlider, &QSlider::valueChanged, this, [this] { debounceApply(); });
    connect(m_plateThickness, &UnitField::edited, this, [this] { apply(false); });
    connect(m_plateIndex, &UnitField::edited, this, [this] { apply(false); });

    build("Optical bench / detector fringes", m_canvas, "Detector intensity vs mirror displacement", "Drag movable M1 or use its slider. ΔOPD = 2d cos θ; λ = 2d/N.");
    useNativeResultCards();
    auto* michelsonHelpBtn = new QToolButton;
    michelsonHelpBtn->setText("?");
    michelsonHelpBtn->setFixedSize(24, 24);
    michelsonHelpBtn->setToolTip("How to use — Michelson Interferometer");
    michelsonHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(michelsonHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Michelson Interferometer",
            "<h3>Michelson Interferometer — laser wavelength</h3>"
            "<p>Coherent light from <bdi dir=\"ltr\">S</bdi> is divided at <bdi dir=\"ltr\">BS</bdi>. Compensating plate <bdi dir=\"ltr\">CP</bdi> matches the glass path through <bdi dir=\"ltr\">BS</bdi>. The reflected arms form virtual images <bdi dir=\"ltr\">M1′</bdi> and <bdi dir=\"ltr\">M2</bdi> separated by <bdi dir=\"ltr\">d</bdi>, and virtual sources <bdi dir=\"ltr\">S1</bdi> and <bdi dir=\"ltr\">S2</bdi> separated by <bdi dir=\"ltr\">2d</bdi>.</p>"
            "<p><b>Move M1:</b> drag the amber mirror vertically or change <bdi dir=\"ltr\">d</bdi> with the slider. Increasing <bdi dir=\"ltr\">d</bdi> expands the circular fringe pattern; decreasing <bdi dir=\"ltr\">d</bdi> contracts them. At <bdi dir=\"ltr\">d = 0</bdi> the center is dark.</p>"
            "<p><b>Wavelength:</b> count <bdi dir=\"ltr\">N</bdi> central fringes while <bdi dir=\"ltr\">M1</bdi> moves by <bdi dir=\"ltr\">d</bdi>, then solve <bdi dir=\"ltr\">λ = 2d/N</bdi>. The mode also solves <bdi dir=\"ltr\">N</bdi> or <bdi dir=\"ltr\">d</bdi> when <bdi dir=\"ltr\">λ</bdi> is known.</p>"
            "<p><b>Total path difference:</b> <bdi dir=\"ltr\">Δ = 2d cos θ + λ/2</bdi>. Bright (constructive) fringes satisfy <bdi dir=\"ltr\">Δ = 2d cos θ + λ/2 = mλ</bdi>; dark (destructive) fringes satisfy <bdi dir=\"ltr\">Δdestr = 2d cos θ + λ/2 = (m + 1/2)λ</bdi>, for <bdi dir=\"ltr\">m = 0, 1, 2, …</bdi>.</p>"
            "<p>Moving <bdi dir=\"ltr\">M1</bdi> by <bdi dir=\"ltr\">λ/4</bdi> changes the round-trip path difference by <bdi dir=\"ltr\">λ/2</bdi>.</p>"
            "<p>The Displacement and Fringes tabs retain the existing medium, visibility, coherence and actuator demonstrations.</p>");
    });
    auto* michelsonHelpBar = new QHBoxLayout;
    michelsonHelpBar->setContentsMargins(0, 0, 0, 0);
    michelsonHelpBar->addStretch(1);
    michelsonHelpBar->addWidget(michelsonHelpBtn);
    m_diagram->body()->addLayout(michelsonHelpBar);
    reset();
}
MichelsonInput MichelsonPage::currentInput() const {
    const double n = m_medium->currentIndex() == m_medium->count() - 1
        ? m_nSlider->value() / 1000.0 : m_medium->currentData().toDouble();
    MichelsonInput input;
    input.lambdaNm = m_lambda->value();
    input.displacementUm = m_dx->value();
    input.n = n;
    input.visibility = m_balance->currentData().toDouble();
    input.fringes = m_tabs->current() == 1;
    input.piezo = m_reference->currentData().toDouble() > 0.5;
    input.coherenceLengthUm = m_coherenceLength->value();
    input.tiltMrad = m_tiltSlider->value() / 1000.0;
    input.plateThicknessUm = m_plateThickness->value();
    input.plateIndex = m_plateIndex->value();
    input.wavelengthMode = m_tabs->current() == 2;
    input.fringeCount = m_fringeCount->value();
    input.solveFor = m_solveFor->currentData().toInt();
    input.viewAngleDeg = m_viewAngle->value();
    return input;
}

ModelReport MichelsonPage::compute(const ReportContext& ctx) {
    return WE::michelson(currentInput(), ctx);
}

void MichelsonPage::setMirrorDisplacement(double displacementUm) {
    const bool wavelengthMode = m_tabs->current() == 2;
    const double minimum = wavelengthMode ? 0.0 : -MichelsonCanvas::kMaxInteractiveTravelUm;
    const double travel = std::clamp(displacementUm, minimum, MichelsonCanvas::kMaxInteractiveTravelUm);
    m_dx->setValue(travel, 4);
    QSignalBlocker blocker(m_dxSlider);
    m_dxSlider->setValue(static_cast<int>(std::lround(travel * 10000.0)));
    debounceApply();
}

void MichelsonPage::updateCanvas() {
    const MichelsonInput input = currentInput();
    const MichelsonDisplayData display = WE::michelsonDisplayData(input);
    m_canvas->setDisplayData(display);
    if (input.wavelengthMode) {
        if (input.solveFor == 0) m_lambda->setValue(display.wavelengthNm, 2);
        else if (input.solveFor == 1) m_fringeCount->setValue(display.fringeCount, 4);
        else if (input.solveFor == 2) m_dx->setValue(display.displacementUm, 4);
        QSignalBlocker blocker(m_dxSlider);
        m_dxSlider->setValue(static_cast<int>(std::lround(display.displacementUm * 10000.0)));
    }
    const bool wavelengthMode = input.wavelengthMode;
    const bool fringes = input.fringes;
    const bool piezo = input.piezo;
    if (wavelengthMode) {
        retitle("Michelson setup / virtual images M1′ and M2", "Central intensity and fringes vs M1 displacement d");
    } else {
        retitle(fringes ? (piezo ? "Optical bench / piezo scan of M2" : "Optical bench / fringe pattern") : "Optical bench / detector fringes",
                fringes ? "Intensity across a ±2 µm M2 sweep" : "Detector intensity vs M2 displacement");
    }
}
void MichelsonPage::resetFields() {
    m_lambda->setValue(632.8, 2);
    // Select the source preset matching the default wavelength (He-Ne · 632.8 nm).
    for (int i = 0; i < m_source->count() - 1; ++i) {
        if (std::fabs(m_source->itemData(i).toDouble() - 632.8) < 1e-9) {
            m_source->setCurrentIndex(i);
            break;
        }
    }
    m_dx->setValue(0.75, 4);
    m_fringeCount->setValue(2.3704, 4);
    m_viewAngle->setValue(0.0, 2);
    m_solveFor->setCurrentIndex(0);
    m_medium->setCurrentIndex(0);                                    // Air · n = 1.0000
    QSignalBlocker blockerN(m_nSlider);
    m_nSlider->setValue(1000);                                       // n = 1.000
    m_reference->setCurrentIndex(0);
    m_balance->setCurrentIndex(0);
    m_tabs->setCurrent(0);
    QSignalBlocker blockerDx(m_dxSlider);
    m_dxSlider->setValue(7500);
    // Items 13-15: reset new parameters to defaults
    QSignalBlocker blockerTilt(m_tiltSlider);
    m_tiltSlider->setValue(0);
    m_coherenceLength->setValue(0.0, 2);
    m_plateThickness->setValue(0.0, 2);
    m_plateIndex->setValue(1.5, 4);
}
QString MichelsonPage::runDescription() const {
    if (m_tabs->current() == 2)
        return QString("M1 d %1 µm · N %2 · solve %3").arg(m_dx->text(), m_fringeCount->text(), m_solveFor->currentText());
    return QString("M2 Δx %1 µm · λ %2 nm · %3").arg(m_dx->text(), m_lambda->text(), m_tabs->current() == 1 ? "fringes" : "displacement");
}

// ============================================================================ thin film
ThinFilmPage::ThinFilmPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Thin-Film Interference", "Single film at normal incidence: reflection and transmission", "Thin-Film Interference", ctx, parent) {
    addTabs({"Reflection", "Transmission", "Colours"});
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Vacuum wavelength · λ", "nm", 600, 2);
    m_lambda->setToolTip("Vacuum wavelength in nm. Constructive reflection at 2nt = (q + ½)λ (one phase reversal).");
    m_n = addField(QString::fromLatin1(FieldIds::refractiveIndex), "Film index · n", "", 1.5, 4, false, true, 0.01);
    m_n->setToolTip("Film refractive index n. Sets optical thickness nt.");
    m_n2 = addField(QString::fromLatin1(FieldIds::filmSubstrateIndex), "Substrate index · n₂", "", 1.5, 4, false, true, 0.01);
    m_n2->setToolTip("Substrate index n₂. n > n₂ gives one phase reversal at top only.");
    m_t = addField(QString::fromLatin1(FieldIds::thickness), "Film thickness · t", "nm", 100, 2, false, true, 5);
    m_t->setToolTip("Physical film thickness in nm. Optical path difference = 2nt.");
    m_theta = addField(QString::fromLatin1(FieldIds::filmIncidenceAngle), "Incidence angle · θ", "°", 0.0, 2, false, true, 1);
    m_theta->setToolTip("Angle of incidence in degrees. 0° = normal incidence.");
    // ---- Preset scenarios (Item 19) ----
    auto* presetGrid = new QGridLayout;
    presetGrid->setHorizontalSpacing(6);
    presetGrid->setVerticalSpacing(6);
    presetGrid->setColumnStretch(0, 1);
    presetGrid->setColumnStretch(1, 1);
    int presetIndex = 0;
    auto makePreset = [&](const QString& label, double lambda, double n, double n2, double tNm, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setToolTip(label);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(b, &QPushButton::clicked, this, [this, lambda, n, n2, tNm, tabIdx] {
            m_syncing = true;
            m_lambda->setValue(lambda, 2);
            m_n->setValue(n, 4);
            m_n2->setValue(n2, 4);
            m_t->setValue(tNm, 2);
            m_theta->setValue(0.0, 2);
            m_tabs->setCurrent(tabIdx);
            m_syncing = false;
            apply(false);
        });
        presetGrid->addWidget(b, presetIndex / 2, presetIndex % 2);
        ++presetIndex;
        return b;
    };
    m_presetSoap = makePreset("Soap · n=1.333", 600.0, 1.333, 1.333, 100.0, 0);
    m_presetOil = makePreset("Oil · n=1.500", 600.0, 1.500, 1.500, 100.0, 0);
    m_presetTransmission = makePreset("Transmission", 600.0, 1.500, 1.500, 100.0, 1);
    addFoldout(m_fieldLayout, "Presets", presetGrid);
    m_swatch = new QLabel;
    m_swatch->setObjectName("colourSwatch");
    m_swatch->setFixedHeight(34);
    m_swatch->setVisible(false);
    m_fieldLayout->addWidget(m_swatch);
    m_canvas = new ThinFilmCanvas;
    build("Film layers / two reflected rays", m_canvas, "Reflected intensity vs thickness", "Reflection: one phase reversal, bright at 2nt = (q + ½)λ. Transmission: no net reversal, bright at 2nt = qλ. Colours: the whole visible band at once.");
    auto* thinFilmHelpBtn = new QToolButton;
    thinFilmHelpBtn->setText("?");
    thinFilmHelpBtn->setFixedSize(24, 24);
    thinFilmHelpBtn->setToolTip("How to use — Thin-Film Interference");
    thinFilmHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(thinFilmHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Thin-Film Interference",
            "<h3>Thin-Film Interference</h3>"
            "<p><b>λ:</b> vacuum wavelength in nm.</p>"
            "<p><b>n:</b> film index; <b>n₂:</b> substrate index. n > n₂ gives one phase reversal at the top surface only.</p>"
            "<p><b>t:</b> film thickness. Optical path difference = 2nt.</p>"
            "<p><b>θ:</b> incidence angle; 0° = normal.</p>"
            "<p>Reflection: bright at 2nt = (q + ½)λ. Transmission: bright at 2nt = qλ.</p>"
            "<p>Presets for soap/oil films and transmission view.</p>");
    });
    auto* thinFilmHelpBar = new QHBoxLayout;
    thinFilmHelpBar->setContentsMargins(0, 0, 0, 0);
    thinFilmHelpBar->addStretch(1);
    thinFilmHelpBar->addWidget(thinFilmHelpBtn);
    m_diagram->body()->addLayout(thinFilmHelpBar);
    reset();
}
ModelReport ThinFilmPage::compute(const ReportContext& ctx) {
    ThinFilmInput in;
    in.lambdaNm = m_lambda->value(); in.n = m_n->value(); in.n2 = m_n2->value();
    in.thicknessNm = m_t->value(); in.thetaDeg = m_theta->value();
    in.transmission = m_tabs->current() == 1;
    in.colours = m_tabs->current() == 2;
    return WE::thinFilm(in, ctx);
}

QString ThinFilmPage::colourSwatch() {
    if (m_tabs->current() != 2) return QString();                    // the swatch only exists on the Colours tab
    const FilmColour c = thinFilmColour(m_t->value(), m_n->value(), m_n2->value());
    return QString::fromStdString(c.hex);
}
void ThinFilmPage::updateCanvas() {
    const int tab = m_tabs->current();
    const bool transmission = tab == 1;
    ThinFilmInput in;
    in.lambdaNm = m_lambda->value(); in.n = m_n->value(); in.n2 = m_n2->value();
    in.thicknessNm = m_t->value(); in.thetaDeg = m_theta->value();
    in.transmission = transmission; in.colours = tab == 2;
    m_canvas->setInput(in);
    retitle(tab == 2 ? "Film layers / colour by thickness" : transmission ? "Film layers / transmitted beam" : "Film layers / two reflected rays",
            tab == 2 ? "Reflectance of the whole spectrum" : transmission ? "Transmitted intensity vs thickness" : "Reflected intensity vs thickness");
    if (m_swatch) {
        const bool colours = tab == 2;
        m_swatch->setVisible(colours);
        if (colours) {
            const FilmColour c = thinFilmColour(in.thicknessNm, in.n, in.n2);
            m_swatch->setStyleSheet(QString("background: %1; border: 1px solid #00000055; border-radius: 4px;").arg(QString::fromStdString(c.hex)));
            m_swatch->setToolTip(c.hueNm > 0 ? QString("%1 nm dominant").arg(c.hueNm, 0, 'f', 0) : QString("neutral grey"));
        }
    }
}
void ThinFilmPage::resetFields() { m_lambda->setValue(600, 2); m_n->setValue(1.5, 4); m_n2->setValue(1.5, 4); m_t->setValue(100, 2); m_theta->setValue(0.0, 2); m_tabs->setCurrent(0); }
// (the Colours tab keeps the same three inputs: λ marks a point, n and t decide the swatch)
QString ThinFilmPage::runDescription() const {
    return QString("t %1 nm · n %2 · n₂ %3 · θ %4° · %5").arg(m_t->text(), m_n->text(), m_n2->text(), m_theta->text(), m_tabs->current() == 1 ? "transmission" : "reflection");
}

// ============================================================================ diffraction grating
GratingPage::GratingPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Diffraction Grating", "Principal maxima of a transmission grating", "Diffraction Grating", ctx, parent) {
    addTabs({"Transmission", "Orders", "Resolution"});
    m_density = addField(QString::fromLatin1(FieldIds::gratingDensity), "Line density · N", "lines/cm", 5000, 0, false, true, 100);
    m_density->setToolTip("Line density in lines/cm. Grating spacing d = 1/N; maxima at d·sin θ = mλ.");
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Wavelength · λ₁", "nm", 600, 2);
    m_lambda->setToolTip("Primary wavelength λ₁ in nm. Angular spread scales with λ/d.");
    m_lambda2 = addField(QString::fromLatin1(FieldIds::gratingLambda2), "Second wavelength · λ₂ (0 = off)", "nm", 0.0, 2);
    m_lambda2->setToolTip("Optional second wavelength λ₂ (nm). Set 0 to disable; two wavelengths show separate order fans.");
    m_order = addField(QString::fromLatin1(FieldIds::order), "Selected order · m", "order", 1, 0, true, true, 1);
    m_order->setToolTip("Diffraction order m. Peak angle from d·sin θ = mλ.");
    m_slits = addField(QString::fromLatin1(FieldIds::illuminatedSlits), "Illuminated slit count · M", "slits", 20, 0, false, true, 1);
    m_slits->setToolTip("Number M of illuminated slits. Resolution R = m·M; peak width ∝ 1/M.");
    m_incidence = addCombo("Incidence", {"Normal · 0.00°", "15.00°", "30.00°"}, {0, 15, 30});
    m_incidence->setToolTip("Angle of incidence; shifts all orders via the grating equation.");
    // ---- Preset scenarios (Item 19) ----
    auto* presetGrid = new QGridLayout;
    presetGrid->setHorizontalSpacing(6);
    presetGrid->setVerticalSpacing(6);
    presetGrid->setColumnStretch(0, 1);
    presetGrid->setColumnStretch(1, 1);
    int presetIndex = 0;
    auto makePreset = [&](const QString& label, double density, double lambda, int order, int slits, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setToolTip(label);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(b, &QPushButton::clicked, this, [this, density, lambda, order, slits, tabIdx] {
            m_syncing = true;
            m_density->setValue(density, 0);
            m_lambda->setValue(lambda, 2);
            m_order->setValue(order, 0);
            m_slits->setValue(slits, 0);
            m_tabs->setCurrent(tabIdx);
            m_syncing = false;
            apply(false);
        });
        presetGrid->addWidget(b, presetIndex / 2, presetIndex % 2);
        ++presetIndex;
        return b;
    };
    m_presetStandard = makePreset("5000 /cm · m=1", 5000.0, 600.0, 1, 20, 0);
    m_presetHighDensity = makePreset("600 /mm · m=2", 6000.0, 532.0, 2, 20, 0);
    m_presetResolution = makePreset("Resolution tab", 5000.0, 600.0, 1, 20, 2);
    addFoldout(m_fieldLayout, "Presets", presetGrid);
    m_canvas = new GratingCanvas;
    build("Grating / angular observation screen", m_canvas, "Diffracted intensity vs angle", "N is line density, not slit count. Transmission: intensity fan. Orders: every peak with its angle. Resolution: R = m·M — raise M to split closer lines.");
    auto* gratingHelpBtn = new QToolButton;
    gratingHelpBtn->setText("?");
    gratingHelpBtn->setFixedSize(24, 24);
    gratingHelpBtn->setToolTip("How to use — Diffraction Grating");
    gratingHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(gratingHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Diffraction Grating",
            "<h3>Diffraction Grating</h3>"
            "<p><b>N:</b> line density in lines/cm. Grating spacing d = 1/N; maxima at d·sin θ = mλ.</p>"
            "<p><b>λ₁ / λ₂:</b> wavelength(s) in nm. Two wavelengths show separate order fans.</p>"
            "<p><b>m:</b> selected diffraction order.</p>"
            "<p><b>M:</b> illuminated slit count. Resolution R = m·M; peak width ∝ 1/M.</p>"
            "<p>Presets for standard/high-density gratings and the resolution tab.</p>");
    });
    auto* gratingHelpBar = new QHBoxLayout;
    gratingHelpBar->setContentsMargins(0, 0, 0, 0);
    gratingHelpBar->addStretch(1);
    gratingHelpBar->addWidget(gratingHelpBtn);
    m_diagram->body()->addLayout(gratingHelpBar);
    reset();
}
ModelReport GratingPage::compute(const ReportContext& ctx) {
    if (m_tabs->current() == 2)                                        // the third tab: R = mN, δλ and the FSR
        return WE::gratingResolution({m_density->value(), m_lambda->value(), std::max(1.0, m_order->value()), m_slits->value()}, ctx);
    return WE::grating({m_density->value(), m_lambda->value(), m_lambda2->value(), m_order->value(), m_slits->value(),
                        m_incidence->currentData().toDouble(), m_tabs->current() == 1}, ctx);
}
void GratingPage::updateCanvas() {
    const int tab = m_tabs->current();
    const bool orders = tab == 1;
    m_canvas->setInput({m_density->value(), m_lambda->value(), m_lambda2->value(), m_order->value(), m_slits->value(),
                        m_incidence->currentData().toDouble(), orders});
    retitle(tab == 2 ? "Grating / two lines one Rayleigh step apart" : orders ? "Grating / every propagating order" : "Grating / angular observation screen",
            tab == 2 ? "Single line and a just-resolved pair" : orders ? "Order angles and where every peak sits" : "Diffracted intensity vs angle");
}
void GratingPage::resetFields() { m_density->setValue(5000, 0); m_lambda->setValue(600, 2); m_lambda2->setValue(0.0, 2); m_order->setValue(1, 0); m_slits->setValue(20, 0); m_incidence->setCurrentIndex(0); m_tabs->setCurrent(0); }
QString GratingPage::runDescription() const {
    const int tab = m_tabs->current();
    return QString("order %1 · N %2 /cm · λ₁ %3 nm · λ₂ %4 nm · %5").arg(m_order->text(), m_density->text(),
                                                   m_lambda->text(), m_lambda2->text(),
                                                   tab == 2 ? "resolution" : tab == 1 ? "orders" : "transmission");
}

// ============================================================================ single slit
SingleSlitPage::SingleSlitPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Single-Slit Diffraction", "First minimum and central maximum width", "Single-Slit Diffraction", ctx, parent) {
    addTabs({"Intensity", "Geometry", "Envelope"});
    m_width = addField(QString::fromLatin1(FieldIds::slitWidth), "Slit width · a", "mm", 0.1, 4, false, true, 0.01);
    m_width->setToolTip("Slit width a in mm. First minimum at sin θ = λ/a; central width ∝ λ/a.");
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Wavelength · λ", "nm", 500, 2);
    m_lambda->setToolTip("Wavelength λ in nm. Sets fringe scale; first minimum at sin θ = λ/a.");
    m_screen = addField(QString::fromLatin1(FieldIds::screenDistance), "Screen distance · L", "m", 1.0, 4);
    m_screen->setToolTip("Screen distance L in m. Linear fringe spacing ≈ L·λ/a.");
    m_separation = addField(QString::fromLatin1(FieldIds::slitSeparation), "Second slit · d (envelope tab)", "mm", 0.4, 4);
    m_separation->setToolTip("Second slit separation d (envelope tab only). Fringes inside the envelope; missing orders when d/a is integer.");
    // ---- Preset scenarios (Item 19) ----
    auto* presetGrid = new QGridLayout;
    presetGrid->setHorizontalSpacing(6);
    presetGrid->setVerticalSpacing(6);
    presetGrid->setColumnStretch(0, 1);
    presetGrid->setColumnStretch(1, 1);
    int presetIndex = 0;
    auto makePreset = [&](const QString& label, double width, double lambda, double screen, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setToolTip(label);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(32);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(b, &QPushButton::clicked, this, [this, width, lambda, screen, tabIdx] {
            m_syncing = true;
            m_width->setValue(width, 4);
            m_lambda->setValue(lambda, 2);
            m_screen->setValue(screen, 4);
            m_tabs->setCurrent(tabIdx);
            m_syncing = false;
            apply(false);
        });
        presetGrid->addWidget(b, presetIndex / 2, presetIndex % 2);
        ++presetIndex;
        return b;
    };
    m_presetStandard = makePreset("a = 0.1 mm", 0.1, 500.0, 1.0, 0);
    m_presetGeometry = makePreset("Geometry", 0.1, 500.0, 1.0, 1);
    m_presetEnvelope = makePreset("Envelope", 0.1, 500.0, 1.0, 2);
    addFoldout(m_fieldLayout, "Presets", presetGrid);
    // Item 11: real-time updates are built into addField() (edited → apply(false))
    m_wave = addCombo("Incident wave", {"Plane · coherent"}, {0});
    m_wave->setToolTip("Incident wave type. Plane coherent wave illuminates the slit(s).");
    m_scale = addCombo("Intensity scale", {"Normalized · I/I₀"}, {0});
    m_scale->setToolTip("Intensity scale. Normalized divides by the central maximum I₀.");
    m_exactGeom = addCombo("Geometry model", {"Exact · tan(asin(λ/a))", "Small-angle · ≈ 2Lλ/a"}, {1, 0});
    m_exactGeom->setToolTip("Geometry model: exact sin/cos vs small-angle approximation for fringe positions.");
    m_showEnvelopeOnly = new QCheckBox("Show sinc² envelope only");
    m_showEnvelopeOnly->setToolTip("When checked, draws just the sinc² envelope (cos² interference term = 1).");
    m_showEnvelopeOnly->setChecked(false);
    m_fieldLayout->addWidget(m_showEnvelopeOnly);
    connect(m_showEnvelopeOnly, &QCheckBox::toggled, this, [this](bool) { apply(false); });
    m_canvas = new SingleSlitCanvas;
    build("Slit / screen diffraction pattern", m_canvas, "Single-slit intensity vs angle", "Minima: a sin θ = mλ. Intensity: the envelope. Geometry: where each minimum lands. Envelope: two slits of width a — fringes inside the envelope, missing orders at d/a.");
    auto* slitHelpBtn = new QToolButton;
    slitHelpBtn->setText("?");
    slitHelpBtn->setFixedSize(24, 24);
    slitHelpBtn->setToolTip("How to use — Single-Slit Diffraction");
    slitHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(slitHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Single-Slit Diffraction",
            "<h3>Single-Slit Diffraction</h3>"
            "<p><b>a:</b> slit width in mm. First minimum at sin θ = λ/a; central width ∝ λ/a.</p>"
            "<p><b>λ:</b> wavelength in nm.</p>"
            "<p><b>L:</b> screen distance in m. Linear spacing ≈ L·λ/a.</p>"
            "<p><b>d:</b> second slit separation (envelope tab). Missing orders when d/a is integer.</p>"
            "<p>Tabs: Intensity, Geometry, Envelope (two-slit finite-width).</p>"
            "<p>Presets for standard/geometry/envelope views.</p>");
    });
    auto* slitHelpBar = new QHBoxLayout;
    slitHelpBar->setContentsMargins(0, 0, 0, 0);
    slitHelpBar->addStretch(1);
    slitHelpBar->addWidget(slitHelpBtn);
    m_diagram->body()->addLayout(slitHelpBar);
    reset();
}
ModelReport SingleSlitPage::compute(const ReportContext& ctx) {
    if (m_tabs->current() == 2) {                                      // third tab: two slits of finite width
        SingleSlitInput in;
        in.widthMm = m_width->value(); in.lambdaNm = m_lambda->value(); in.screenM = m_screen->value();
        in.separationMm = m_separation->value(); in.envelope = true;
        // Item 12: show sinc² envelope only when checkbox is checked
        if (m_showEnvelopeOnly->isChecked()) {
            // Override: show just the envelope (cos² term = 1)
            in.separationMm = 0.0;  // will be caught by d > a check, so handle separately
            return WE::singleSlit({m_width->value(), m_lambda->value(), m_screen->value(), false, false, 0.0, m_exactGeom->currentData().toInt() == 1}, ctx);
        }
        return WE::doubleSlitEnvelope(in, ctx);
    }
    return WE::singleSlit({m_width->value(), m_lambda->value(), m_screen->value(), m_tabs->current() == 1, false, m_separation->value(),
                           m_exactGeom->currentData().toInt() == 1}, ctx);
}
void SingleSlitPage::updateCanvas() {
    const int tab = m_tabs->current();
    const bool geometry = tab == 1;
    const bool exact = m_exactGeom->currentData().toInt() == 1;
    SingleSlitInput in;
    in.widthMm = m_width->value(); in.lambdaNm = m_lambda->value(); in.screenM = m_screen->value();
    in.geometry = geometry; in.envelope = tab == 2; in.separationMm = m_separation->value(); in.exactGeometry = exact;
    m_canvas->setInput(in);
    retitle(tab == 2 ? "Two slits / fringes inside the envelope" : geometry ? "Slit / where the minima land" : "Slit / screen diffraction pattern",
            tab == 2 ? "Fringes and the slit envelope" : geometry ? "Minimum position vs order" : "Single-slit intensity vs angle");
}
void SingleSlitPage::resetFields() { m_width->setValue(0.1, 4); m_lambda->setValue(500, 2); m_screen->setValue(1.0, 4); m_separation->setValue(0.4, 4); m_wave->setCurrentIndex(0); m_scale->setCurrentIndex(0); m_exactGeom->setCurrentIndex(0); m_showEnvelopeOnly->setChecked(false); m_tabs->setCurrent(0); }
QString SingleSlitPage::runDescription() const {
    const int tab = m_tabs->current();
    return QString("a %1 mm · λ %2 nm · %3 · %4 · env%5").arg(m_width->text(), m_lambda->text(),
                                                 tab == 2 ? "envelope" : tab == 1 ? "geometry" : "intensity",
                                                 m_exactGeom->currentText().section(QString(" ·"), 0, 0),
                                                 m_showEnvelopeOnly->isChecked() ? "only" : "fringes");
}

// ============================================================================ Malus + Brewster
PolarizationPage::PolarizationPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Polarization (Malus & Brewster)", "Polarizers and the Brewster angle", "Polarization (Malus & Brewster)", ctx, parent) {
    addTabs({"Malus", "Brewster"});
    m_beam = addCombo("Incident beam", {"Unpolarized", "Polarized"}, {1, 0});
    m_beam->setToolTip("Incident beam: unpolarized (half intensity through first polarizer) or already polarized.");
    m_intensity = addField(QString::fromLatin1(FieldIds::incidentIntensity), "Before polarizer · Iᵢₙ", "W/m²", 100, 2);
    m_intensity->setToolTip("Incident intensity I_in in W/m² before the first polarizer.");
    m_angle = addField(QString::fromLatin1(FieldIds::relativeAngle), "Relative axis angle · θ", "°", 60, 2, false, true, 1);
    m_angle->setToolTip("Relative angle θ between polarizer axes in °. Malus: I = I₀·cos²θ for polarized input.");
    m_n1 = addField(QString::fromLatin1(FieldIds::refractiveIndex1), "Incident index · n₁ (air)", "", 1.0, 4, false, true, 0.01);
    m_n1->setToolTip("Incident medium index n₁ (air ≈ 1.0). Brewster: tan θ_B = n₂/n₁.");
    m_n2 = addField(QString::fromLatin1(FieldIds::refractiveIndex2), "Transmitted index · n₂", "", 1.5, 4, false, true, 0.01);
    m_n2->setToolTip("Transmitted medium index n₂. Brewster angle θ_B = atan(n₂/n₁), where R_p = 0.");
    // ---- Preset scenarios (Item 19) ----
    auto* presetGrid = new QGridLayout;
    presetGrid->setHorizontalSpacing(6);
    presetGrid->setVerticalSpacing(6);
    presetGrid->setColumnStretch(0, 1);
    presetGrid->setColumnStretch(1, 1);
    int presetIndex = 0;
    auto makePreset = [&](const QString& label, double intensity, double angle, double n1, double n2, bool unpolarized, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setToolTip(label);
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(b, &QPushButton::clicked, this, [this, intensity, angle, n1, n2, unpolarized, tabIdx] {
            m_syncing = true;
            m_intensity->setValue(intensity, 2);
            m_angle->setValue(angle, 2);
            m_n1->setValue(n1, 4);
            m_n2->setValue(n2, 4);
            m_beam->setCurrentIndex(unpolarized ? 0 : 1);
            m_tabs->setCurrent(tabIdx);
            m_syncing = false;
            apply(false);
        });
        presetGrid->addWidget(b, presetIndex / 2, presetIndex % 2);
        ++presetIndex;
        return b;
    };
    m_presetMalus = makePreset("Malus · θ=60°", 100.0, 60.0, 1.0, 1.5, true, 0);
    m_presetBrewster = makePreset("Brewster · air→glass", 100.0, 0.0, 1.0, 1.5, true, 1);
    addFoldout(m_fieldLayout, "Presets", presetGrid);
    m_canvas = new PolarizationCanvas;
    build("Two polarizers / independent Brewster reference", m_canvas, "Intensity vs relative angle", "Malus: θ range 0–90°, the first polarizer transmits half of unpolarized light. Brewster: tan θB = n₂/n₁, R_p = 0 at θB.");
    auto* polHelpBtn = new QToolButton;
    polHelpBtn->setText("?");
    polHelpBtn->setFixedSize(24, 24);
    polHelpBtn->setToolTip("How to use — Polarization");
    polHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(polHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Polarization",
            "<h3>Polarization (Malus &amp; Brewster)</h3>"
            "<p><b>Beam:</b> unpolarized (half through first polarizer) or polarized.</p>"
            "<p><b>I_in:</b> incident intensity in W/m² before the first polarizer.</p>"
            "<p><b>θ:</b> relative axis angle in °. Malus: I = I₀·cos²θ for polarized input.</p>"
            "<p><b>n₁ / n₂:</b> incident and transmitted indices. Brewster: tan θ_B = n₂/n₁, R_p = 0 at θ_B.</p>"
            "<p>Tabs: Malus law, Brewster interface.</p>"
            "<p>Presets for Malus θ=60° and Brewster air→glass.</p>");
    });
    auto* polHelpBar = new QHBoxLayout;
    polHelpBar->setContentsMargins(0, 0, 0, 0);
    polHelpBar->addStretch(1);
    polHelpBar->addWidget(polHelpBtn);
    m_diagram->body()->addLayout(polHelpBar);
    reset();
}
ModelReport PolarizationPage::compute(const ReportContext& ctx) {
    return WE::polarization({m_intensity->value(), m_angle->value(), m_n1->value(), m_n2->value(), m_beam->currentData().toDouble() > 0.5,
                             m_tabs->current() == 1}, ctx);
}
void PolarizationPage::updateCanvas() {
    const bool brewster = m_tabs->current() == 1;
    m_canvas->setInput({m_intensity->value(), m_angle->value(), m_n1->value(), m_n2->value(), m_beam->currentData().toDouble() > 0.5, brewster});
    retitle(brewster ? "Interface at Brewster / reflected beam s-polarized" : "Two polarizers / independent Brewster reference",
            brewster ? "p-polarized reflectance vs incidence" : "Intensity vs relative angle");
}
void PolarizationPage::resetFields() { m_beam->setCurrentIndex(0); m_intensity->setValue(100, 2); m_angle->setValue(60, 2); m_n1->setValue(1.0, 4); m_n2->setValue(1.5, 4); m_tabs->setCurrent(0); }
QString PolarizationPage::runDescription() const {
    return QString("θ %1° · Iᵢₙ %2 W/m² · %3").arg(m_angle->text(), m_intensity->text(), m_tabs->current() == 1 ? "brewster" : "malus");
}

} // namespace phy104
