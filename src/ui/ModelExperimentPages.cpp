#include "ModelExperimentPages.h"
#include "AppContext.h"
#include "FieldIds.h"
#include "Components.h"
#include "OpticsCanvases.h"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QVBoxLayout>
#include <cmath>

namespace phy104 {
namespace WE = WaveExperiments;
namespace {
QString q(const std::string& s) { return QString::fromStdString(s); }
}

// ============================================================================ ModelPage
ModelPage::ModelPage(const QString& title, const QString& subtitle, const QString& experiment, AppContext* ctx, QWidget* parent)
    : ExperimentPage(title, subtitle, experiment, ctx, parent) {
    useSaveExportHeader();
    m_controls = new Panel("Experiment controls");
    m_controls->body()->setSpacing(10);
    m_fieldLayout = m_controls->body();
}

UnitField* ModelPage::addField(const QString& id, const QString& label, const QString& unit, double value, int decimals, bool signedValue, bool spinner, double step) {
    auto* f = new UnitField(unit);
    f->setShowPlus(signedValue);
    if (spinner) f->setSpinner(true, step);
    f->setValue(value, decimals);
    bind(id, f);
    m_fieldLayout->addWidget(labeled(label, f));
    connect(f, &UnitField::edited, this, [this] { apply(false); });
    return f;
}

QComboBox* ModelPage::addCombo(const QString& label, const QStringList& items, const QList<double>& values) {
    auto* c = new QComboBox;
    for (int i = 0; i < items.size(); ++i) c->addItem(items[i], values.value(i, 0.0));
    m_fieldLayout->addWidget(labeled(label, c));
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
    m_reg1 = new RegisterView;
    m_reg2 = new RegisterView;
    m_interpretation = new NoticeCard;
    m_guard = new NoticeCard;
    m_results->body()->addWidget(m_reg1);
    m_results->body()->addWidget(m_reg2);
    m_results->body()->addWidget(m_interpretation);
    m_results->body()->addWidget(m_guard);
    cols.right->addWidget(m_results);
    cols.right->addStretch();
    setPanels(m_diagram, m_chartPanel, m_results, "", "");
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
    m_chart->setLegend(legends, q(c.legend));
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
    showRegisters(m_reg1, m_reg2, r);
    if (r.paused) {                                                    // the red "paused" card replaces the interpretation
        m_interpretation->hide();
        m_guard->setNotice(r.notices[0]);
    } else {
        m_interpretation->show();
        m_interpretation->setNotice(r.notices[0]);
        m_guard->setNotice(r.notices.size() > 1 ? r.notices[1] : r.notices[0]);
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
    m_radius = addField(QString::fromLatin1(FieldIds::radiusOfCurvature), "Radius of curvature · R", "m", 1.0, 4);
    m_order = addField(QString::fromLatin1(FieldIds::order), "Selected dark-ring order · m", "integer", 4, 0, false, true, 1);
    m_sigmaLambda = addField(QString::fromLatin1(FieldIds::sigmaNewtonLambda), "σ(λ) uncertainty", "nm", 0.0, 4);
    m_sigmaRadius = addField(QString::fromLatin1(FieldIds::sigmaNewtonRadius), "σ(R) uncertainty", "m", 0.0, 4);
    m_sigmaN = addField(QString::fromLatin1(FieldIds::sigmaNewtonIndex), "σ(n) uncertainty", "", 0.0, 4);
    m_medium = addCombo("Film medium", {"Air · n = 1.0000", "Water · n = 1.3330", "Oil · n = 1.4000"}, {1.0, 1.333, 1.4});
    m_viewing = addCombo("Viewing mode", {"Reflected light", "Transmitted light"}, {0, 1});
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    m_fieldLayout->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double lambda, double radius, int order, int mediumIdx, int viewingIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
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
        presetRow->addWidget(b);
        return b;
    };
    m_presetAir = makePreset("Air · m=4", 600.0, 1.0, 4, 0, 0);
    m_presetWater = makePreset("Water · m=4", 600.0, 1.0, 4, 1, 0);
    m_presetOil = makePreset("Oil · m=4", 600.0, 1.0, 4, 2, 0);
    m_presetTransmitted = makePreset("Transmitted · m=4", 600.0, 1.0, 4, 0, 1);
    m_fieldLayout->addLayout(presetRow);
    m_canvas = new NewtonCanvas;
    build("Lens on plate / concentric reflected rings", m_canvas, "Dark-ring radius squared vs order", "Reflected: m = 0 is the dark contact center. Transmitted: it is bright and the dark rings sit at half orders. t ≈ r²/(2R).");
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
    : ModelPage("Michelson Interferometry", "Drag movable M2 to follow the fringe shift", "Michelson Interferometry", ctx, parent) {
    addTabs({"Displacement", "Fringes"});
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Wavelength · λ", "nm", 632.8, 2);

    m_source = new QComboBox;
    for (const auto& s : ctx->presets.sources) m_source->addItem(QString::fromStdString(s.name), s.wavelengthNm);
    m_source->addItem("Custom", -1.0);
    m_fieldLayout->addWidget(labeled("Source preset", m_source));

    m_dx = addField(QString::fromLatin1(FieldIds::mirrorDisplacement), "Mirror displacement · Δx", "µm", 0.75, 4, true, true, 0.05);

    m_dxSlider = new QSlider(Qt::Horizontal);
    m_dxSlider->setObjectName("michelsonMirrorSlider");
    const int sliderLimit = static_cast<int>(std::lround(MichelsonCanvas::kMaxInteractiveTravelUm * 10000.0));
    m_dxSlider->setRange(-sliderLimit, sliderLimit);                  // 0.1 nm steps, ±2 µm stage travel
    m_dxSlider->setSingleStep(10);                                    // 1 nm keyboard/page adjustment
    m_dxSlider->setPageStep(1000);
    m_dxSlider->setTickPosition(QSlider::TicksBelow);
    m_dxSlider->setTickInterval(5000);
    m_dxSlider->setAccessibleName("M2 mirror travel in micrometres");
    m_dxSlider->setToolTip("Drag the amber M2 mirror left or right along its arm, or use this slider. Range: −2 to +2 µm.");
    m_fieldLayout->addWidget(labeled("M2 travel · ±2 µm", m_dxSlider));

    m_medium = new QComboBox;
    for (const auto& m : ctx->presets.media) m_medium->addItem(QString::fromStdString(m.name), m.n);
    m_medium->addItem("Custom · n = slider", -1.0);
    m_fieldLayout->addWidget(labeled("Arm medium · n", m_medium));

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
    m_fieldLayout->addWidget(labeled("Index n · 1.000–2.500", m_nSlider));

    m_reference = addCombo("M2 drive", {"Manual micrometer", "Piezo actuator"}, {0, 1});
    m_balance = addCombo("Beam balance", {"Equal beams · V = 1.00", "Unequal beams · V = 0.60"}, {1.0, 0.6});
    // Item 13: coherence length / source linewidth
    m_coherenceLength = addField(QString::fromLatin1(FieldIds::michelsonCoherenceLength), "Coherence length · Lc", "µm", 0.0, 2, false, true, 1);
    // Item 14: mirror tilt
    m_tiltSlider = new QSlider(Qt::Horizontal);
    m_tiltSlider->setObjectName("michelsonTiltSlider");
    m_tiltSlider->setRange(0, 1000);           // 0 to 10 mrad in 0.01 mrad steps
    m_tiltSlider->setSingleStep(10);
    m_tiltSlider->setPageStep(100);
    m_tiltSlider->setValue(0);
    m_tiltSlider->setAccessibleName("M2 mirror tilt in milliradians");
    m_tiltSlider->setToolTip("Tilt M2 to change from straight fringes (tilt = 0) to circular bullseye fringes.");
    m_fieldLayout->addWidget(labeled("Mirror tilt · mrad", m_tiltSlider));
    // Item 15: glass plate in one arm
    m_plateThickness = addField(QString::fromLatin1(FieldIds::michelsonPlateThickness), "Plate thickness · t", "µm", 0.0, 2, false, true, 1);
    m_plateIndex = addField(QString::fromLatin1(FieldIds::michelsonPlateIndex), "Plate index · n_p", "", 1.5, 4, false, true, 0.01);
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    m_fieldLayout->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double lambda, double dxUm, double nIdx, bool fringes) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
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
        presetRow->addWidget(b);
        return b;
    };
    m_presetAir = makePreset("Air · Δx 0.75", 632.8, 0.75, 0, false);
    m_presetCrown = makePreset("Crown · Δx 0.75", 632.8, 0.75, 1, false);
    m_presetFlint = makePreset("Flint · Δx 0.75", 632.8, 0.75, 2, false);
    m_presetFringes = makePreset("Fringes tab", 632.8, 0.75, 0, true);
    presetRow->addStretch(1);
    m_fieldLayout->addLayout(presetRow);
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
    connect(m_canvas, &MichelsonCanvas::mirrorDisplacementChanged, this, &MichelsonPage::setMirrorDisplacement);

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
        apply(false);
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
    connect(m_tiltSlider, &QSlider::valueChanged, this, [this] { apply(false); });
    connect(m_plateThickness, &UnitField::edited, this, [this] { apply(false); });
    connect(m_plateIndex, &UnitField::edited, this, [this] { apply(false); });

    build("Optical bench / detector fringes", m_canvas, "Detector intensity vs mirror displacement", "Drag movable M2 or use its slider. ΔOPD = 2nΔx; one fringe per λ/(2n) of mirror travel.");
    reset();
}
ModelReport MichelsonPage::compute(const ReportContext& ctx) {
    const double n = m_medium->currentIndex() == m_medium->count() - 1
        ? m_nSlider->value() / 1000.0
        : m_medium->currentData().toDouble();
    return WE::michelson({m_lambda->value(), m_dx->value(), n, m_balance->currentData().toDouble(),
                          m_tabs->current() == 1, m_reference->currentData().toDouble() > 0.5,
                          m_coherenceLength->value(),
                          m_tiltSlider->value() / 1000.0,
                          m_plateThickness->value(), m_plateIndex->value()}, ctx);
}
void MichelsonPage::setMirrorDisplacement(double displacementUm) {
    const double travel = std::clamp(displacementUm, -MichelsonCanvas::kMaxInteractiveTravelUm,
                                     MichelsonCanvas::kMaxInteractiveTravelUm);
    m_dx->setValue(travel, 4);
    QSignalBlocker blocker(m_dxSlider);
    m_dxSlider->setValue(static_cast<int>(std::lround(travel * 10000.0)));
    apply(false);
}
void MichelsonPage::updateCanvas() {
    const bool fringes = m_tabs->current() == 1;
    const bool piezo = m_reference->currentData().toDouble() > 0.5;
    const double n = m_medium->currentIndex() == m_medium->count() - 1
        ? m_nSlider->value() / 1000.0
        : m_medium->currentData().toDouble();
    m_canvas->setInput({m_lambda->value(), m_dx->value(), n, m_balance->currentData().toDouble(), fringes, piezo,
                        m_coherenceLength->value(), m_tiltSlider->value() / 1000.0,
                        m_plateThickness->value(), m_plateIndex->value()});
    retitle(fringes ? (piezo ? "Optical bench / piezo scan of M2" : "Optical bench / fringe pattern") : "Optical bench / detector fringes",
            fringes ? "Intensity across a ±2 µm M2 sweep" : "Detector intensity vs M2 displacement");
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
    return QString("M2 Δx %1 µm · λ %2 nm · %3").arg(m_dx->text(), m_lambda->text(), m_tabs->current() == 1 ? "fringes" : "displacement");
}

// ============================================================================ thin film
ThinFilmPage::ThinFilmPage(AppContext* ctx, QWidget* parent)
    : ModelPage("Thin-Film Interference", "Single film at normal incidence: reflection and transmission", "Thin-Film Interference", ctx, parent) {
    addTabs({"Reflection", "Transmission", "Colours"});
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Vacuum wavelength · λ", "nm", 600, 2);
    m_n = addField(QString::fromLatin1(FieldIds::refractiveIndex), "Film index · n", "", 1.5, 4, false, true, 0.01);
    m_n2 = addField(QString::fromLatin1(FieldIds::filmSubstrateIndex), "Substrate index · n₂", "", 1.5, 4, false, true, 0.01);
    m_t = addField(QString::fromLatin1(FieldIds::thickness), "Film thickness · t", "nm", 100, 2, false, true, 5);
    m_theta = addField(QString::fromLatin1(FieldIds::filmIncidenceAngle), "Incidence angle · θ", "°", 0.0, 2, false, true, 1);
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    m_fieldLayout->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double lambda, double n, double n2, double tNm, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
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
        presetRow->addWidget(b);
        return b;
    };
    m_presetSoap = makePreset("Soap · n=1.333", 600.0, 1.333, 1.333, 100.0, 0);
    m_presetOil = makePreset("Oil · n=1.500", 600.0, 1.500, 1.500, 100.0, 0);
    m_presetTransmission = makePreset("Transmission", 600.0, 1.500, 1.500, 100.0, 1);
    m_fieldLayout->addLayout(presetRow);
    m_swatch = new QLabel;
    m_swatch->setObjectName("colourSwatch");
    m_swatch->setFixedHeight(34);
    m_swatch->setVisible(false);
    m_fieldLayout->addWidget(m_swatch);
    m_canvas = new ThinFilmCanvas;
    build("Film layers / two reflected rays", m_canvas, "Reflected intensity vs thickness", "Reflection: one phase reversal, bright at 2nt = (q + ½)λ. Transmission: no net reversal, bright at 2nt = qλ. Colours: the whole visible band at once.");
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
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Wavelength · λ₁", "nm", 600, 2);
    m_lambda2 = addField(QString::fromLatin1(FieldIds::gratingLambda2), "Second wavelength · λ₂ (0 = off)", "nm", 0.0, 2);
    m_order = addField(QString::fromLatin1(FieldIds::order), "Selected order · m", "order", 1, 0, true, true, 1);
    m_slits = addField(QString::fromLatin1(FieldIds::illuminatedSlits), "Illuminated slit count · M", "slits", 20, 0, false, true, 1);
    m_incidence = addCombo("Incidence", {"Normal · 0.00°", "15.00°", "30.00°"}, {0, 15, 30});
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    m_fieldLayout->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double density, double lambda, int order, int slits, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
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
        presetRow->addWidget(b);
        return b;
    };
    m_presetStandard = makePreset("5000 /cm · m=1", 5000.0, 600.0, 1, 20, 0);
    m_presetHighDensity = makePreset("600 /mm · m=2", 6000.0, 532.0, 2, 20, 0);
    m_presetResolution = makePreset("Resolution tab", 5000.0, 600.0, 1, 20, 2);
    m_fieldLayout->addLayout(presetRow);
    m_canvas = new GratingCanvas;
    build("Grating / angular observation screen", m_canvas, "Diffracted intensity vs angle", "N is line density, not slit count. Transmission: intensity fan. Orders: every peak with its angle. Resolution: R = m·M — raise M to split closer lines.");
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
    m_lambda = addField(QString::fromLatin1(FieldIds::wavelength), "Wavelength · λ", "nm", 500, 2);
    m_screen = addField(QString::fromLatin1(FieldIds::screenDistance), "Screen distance · L", "m", 1.0, 4);
    m_separation = addField(QString::fromLatin1(FieldIds::slitSeparation), "Second slit · d (envelope tab)", "mm", 0.4, 4);
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    m_fieldLayout->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double width, double lambda, double screen, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        connect(b, &QPushButton::clicked, this, [this, width, lambda, screen, tabIdx] {
            m_syncing = true;
            m_width->setValue(width, 4);
            m_lambda->setValue(lambda, 2);
            m_screen->setValue(screen, 4);
            m_tabs->setCurrent(tabIdx);
            m_syncing = false;
            apply(false);
        });
        presetRow->addWidget(b);
        return b;
    };
    m_presetStandard = makePreset("a=0.1mm · λ=500", 0.1, 500.0, 1.0, 0);
    m_presetGeometry = makePreset("Geometry tab", 0.1, 500.0, 1.0, 1);
    m_presetEnvelope = makePreset("Envelope tab", 0.1, 500.0, 1.0, 2);
    m_fieldLayout->addLayout(presetRow);
    // Item 11: real-time updates are built into addField() (edited → apply(false))
    m_wave = addCombo("Incident wave", {"Plane · coherent"}, {0});
    m_scale = addCombo("Intensity scale", {"Normalized · I/I₀"}, {0});
    m_exactGeom = addCombo("Geometry model", {"Exact · tan(asin(λ/a))", "Small-angle · ≈ 2Lλ/a"}, {1, 0});
    m_showEnvelopeOnly = new QCheckBox("Show sinc² envelope only");
    m_showEnvelopeOnly->setChecked(false);
    m_fieldLayout->addWidget(m_showEnvelopeOnly);
    connect(m_showEnvelopeOnly, &QCheckBox::toggled, this, [this](bool) { apply(false); });
    m_canvas = new SingleSlitCanvas;
    build("Slit / screen diffraction pattern", m_canvas, "Single-slit intensity vs angle", "Minima: a sin θ = mλ. Intensity: the envelope. Geometry: where each minimum lands. Envelope: two slits of width a — fringes inside the envelope, missing orders at d/a.");
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
    m_intensity = addField(QString::fromLatin1(FieldIds::incidentIntensity), "Before polarizer · Iᵢₙ", "W/m²", 100, 2);
    m_angle = addField(QString::fromLatin1(FieldIds::relativeAngle), "Relative axis angle · θ", "°", 60, 2, false, true, 1);
    m_n1 = addField(QString::fromLatin1(FieldIds::refractiveIndex1), "Incident index · n₁ (air)", "", 1.0, 4, false, true, 0.01);
    m_n2 = addField(QString::fromLatin1(FieldIds::refractiveIndex2), "Transmitted index · n₂", "", 1.5, 4, false, true, 0.01);
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    m_fieldLayout->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double intensity, double angle, double n1, double n2, bool unpolarized, int tabIdx) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
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
        presetRow->addWidget(b);
        return b;
    };
    m_presetMalus = makePreset("Malus · θ=60°", 100.0, 60.0, 1.0, 1.5, true, 0);
    m_presetBrewster = makePreset("Brewster · air→glass", 100.0, 0.0, 1.0, 1.5, true, 1);
    m_fieldLayout->addLayout(presetRow);
    m_canvas = new PolarizationCanvas;
    build("Two polarizers / independent Brewster reference", m_canvas, "Intensity vs relative angle", "Malus: θ range 0–90°, the first polarizer transmits half of unpolarized light. Brewster: tan θB = n₂/n₁, R_p = 0 at θB.");
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
