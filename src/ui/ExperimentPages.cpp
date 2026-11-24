#include "ExperimentPages.h"
#include "GuidedLabDialog.h"
#include "FieldIds.h"
#include "AppContext.h"
#include "Components.h"
#include "OpticsCanvases.h"
#include "PhysicsCalculator.h"
#include "ThemeManager.h"
#include "Uncertainty.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QSizePolicy>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace phy104 {
namespace ER = ExperimentReports;
namespace PC = PhysicsCalculator;
namespace {

QString q(const std::string& s) { return QString::fromStdString(s); }
QString signedText(double v, int dp = 2) { return (v > 0 ? "+" : "") + QString::number(v, 'f', dp); }

/// "Glass · crown" -> "Glass"
QString shortName(const QString& name) { return name.section(" · ", 0, 0); }

QComboBox* mediaCombo(const OpticsPresets& presets, bool includeAir) {
    auto* c = new QComboBox;
    for (const auto& m : presets.media) {
        if (!includeAir && m.id == "air") continue;
        c->addItem(q(m.name), q(m.id));
    }
    c->addItem("Custom", QString());
    return c;
}

/// Axis range with "nice" ticks: [lo, hi] extended so that `intervals` equal steps hit round numbers.
void niceRange(double lo, double hi, int intervals, double& outLo, double& outHi) {
    if (hi - lo < 1e-9) { lo -= 1; hi += 1; }
    const double raw = (hi - lo) / intervals;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    double step = mag;
    for (double m : {1.0, 2.0, 2.5, 5.0, 10.0}) { step = m * mag; if (step >= raw) break; }
    outLo = std::floor(lo / step) * step;
    outHi = outLo + step * intervals;
    while (outHi < hi - 1e-9) { outHi += step; outLo = outHi - step * intervals; }
}

QWidget* uncertaintyInputs(const std::vector<std::pair<QString, UnitField*>>& inputs) {
    auto* root = new QWidget;
    auto* layout = new QVBoxLayout(root);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    auto* toggle = new QToolButton;
    toggle->setObjectName("uncertaintyToggle");
    toggle->setText("Measurement uncertainties · 1σ");
    toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toggle->setArrowType(Qt::RightArrow);
    toggle->setCheckable(true);
    auto* editor = new QWidget;
    editor->setObjectName("uncertaintyInputs");
    auto* fields = new QVBoxLayout(editor);
    fields->setContentsMargins(12, 2, 0, 0);
    fields->setSpacing(6);
    auto* hint = new QLabel("Independent standard uncertainties; enter 0 when a value is treated as exact.");
    hint->setObjectName("hint");
    hint->setWordWrap(true);
    fields->addWidget(hint);
    for (const auto& [label, field] : inputs) {
        field->setToolTip("Standard uncertainty (1σ). Enter 0 to treat this quantity as exact.");
        fields->addWidget(labeled(label, field));
    }
    editor->hide();
    layout->addWidget(toggle);
    layout->addWidget(editor);
    QObject::connect(toggle, &QToolButton::toggled, editor, [toggle, editor](bool open) {
        toggle->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
        editor->setVisible(open);
    });
    return root;
}

} // namespace

// ============================================================================ Refraction & TIR
RefractionPage::RefractionPage(AppContext* ctx, QWidget* parent)
    : ExperimentPage("Refraction & TIR", "Snell's law and the critical angle", "Refraction & TIR", ctx, parent) {
    auto cols = addThreeColumns();
    auto* controls = new Panel("Experiment controls");
    controls->body()->setSpacing(10);
    cols.left->addWidget(controls, 1);

    m_mode = new ModeTabs({"Refraction", "TIR"});
    m_med1 = mediaCombo(ctx->presets, true);
    m_med2 = mediaCombo(ctx->presets, true);
    m_n1 = new UnitField;  m_n1->setSpinner(true, 0.01);
    m_n2 = new UnitField;  m_n2->setSpinner(true, 0.01);
    m_theta = new UnitField("°");
    m_sigmaN1 = new UnitField; m_sigmaN2 = new UnitField; m_sigmaTheta = new UnitField("°");
    m_sigmaN1->setObjectName(FieldIds::sigmaRefractiveIndex1);
    m_sigmaN2->setObjectName(FieldIds::sigmaRefractiveIndex2);
    m_sigmaTheta->setObjectName(FieldIds::sigmaIncidentAngle);
    bind(QString::fromLatin1(FieldIds::refractiveIndex1), m_n1); bind(QString::fromLatin1(FieldIds::refractiveIndex2), m_n2); bind(QString::fromLatin1(FieldIds::incidentAngle), m_theta);
    bind(QString::fromLatin1(FieldIds::sigmaRefractiveIndex1), m_sigmaN1); bind(QString::fromLatin1(FieldIds::sigmaRefractiveIndex2), m_sigmaN2);
    bind(QString::fromLatin1(FieldIds::sigmaIncidentAngle), m_sigmaTheta);
    auto* f = controls->body();
    f->addWidget(m_mode);
    m_mode->setToolTip("Refraction: Snell's law with two media.  TIR: total internal reflection (requires n₁ > n₂).");
    auto* guidedLab = new QPushButton("Start guided lab");
    guidedLab->setObjectName("guidedLabLaunch");
    guidedLab->setCursor(Qt::PointingHandCursor);
    guidedLab->setToolTip("Follow a short prediction, simulation and reflection activity.");
    f->addWidget(guidedLab);
    f->addWidget(labeled("Incident medium", m_med1));
    m_med1->setToolTip("Choose the incident medium preset, or Custom to enter n₁ manually.");
    f->addWidget(labeled("Refractive index · n₁", m_n1));
    m_n1->setToolTip("Refractive index of the incident medium (e.g. 1.50 for crown glass).");
    f->addWidget(labeled("Transmitted medium", m_med2));
    m_med2->setToolTip("Choose the transmitted medium preset, or Custom to enter n₂ manually.");
    f->addWidget(labeled("Refractive index · n₂", m_n2));
    m_n2->setToolTip("Refractive index of the transmitted medium (e.g. 1.00 for air).");
    f->addWidget(labeled("Incidence angle · θ₁", m_theta));
    m_theta->setToolTip("Angle of incidence measured from the surface normal, in degrees (0–90).");
    f->addWidget(uncertaintyInputs({{"σ(n₁)", m_sigmaN1}, {"σ(n₂)", m_sigmaN2}, {"σ(θ₁)", m_sigmaTheta}}));
    m_sigmaN1->setToolTip("Standard uncertainty (1σ) on n₁. Enter 0 to treat as exact.");
    m_sigmaN2->setToolTip("Standard uncertainty (1σ) on n₂. Enter 0 to treat as exact.");
    m_sigmaTheta->setToolTip("Standard uncertainty (1σ) on θ₁ in degrees. Enter 0 to treat as exact.");
    buildButtons(f, "SI units · angles in degrees");

    // ---- Preset scenarios ---
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    f->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double n1Val, double n2Val, double thetaVal, bool tir) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        connect(b, &QPushButton::clicked, this, [this, n1Val, n2Val, thetaVal, tir] {
            m_mode->setCurrent(tir ? 1 : 0);
            m_n1->setValue(n1Val, 2);
            m_n2->setValue(n2Val, 2);
            m_theta->setValue(thetaVal, 2);
            apply(false);
        });
        presetRow->addWidget(b);
        return b;
    };
    (void)makePreset("Air→Glass · 30°", 1.0, 1.5, 30.0, false);
    (void)makePreset("Glass→Air · 30°", 1.5, 1.0, 30.0, false);
    (void)makePreset("TIR · 50°", 1.5, 1.0, 50.0, true);
    (void)makePreset("Water→Air · 45°", 1.333, 1.0, 45.0, false);
    presetRow->addStretch(1);
    f->addLayout(presetRow);

    m_canvasStack = new QStackedWidget;
    m_canvas = new RefractionCanvas;
    m_tirCanvas = new TirCanvas;
    m_canvasStack->addWidget(m_canvas);
    m_canvasStack->addWidget(m_tirCanvas);
    m_diagram = new Panel("Ray diagram / two incidence cases", "LIVE · 1:1");
    m_diagram->body()->setContentsMargins(0, 0, 0, 0);
    auto* refrHelpBtn = new QToolButton;
    refrHelpBtn->setText("?");
    refrHelpBtn->setFixedSize(24, 24);
    refrHelpBtn->setToolTip("How to use — Refraction & TIR");
    refrHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(refrHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Refraction & TIR",
            "<h3>Refraction &amp; TIR</h3>"
            "<p><b>n₁ / n₂:</b> refractive indices of the two media. Light bends toward the normal when entering a denser medium (n₂ &gt; n₁), away when entering a rarer medium (n₂ &lt; n₁).</p>"
            "<p><b>θ₁:</b> angle of incidence measured from the surface normal, in degrees.</p>"
            "<p><b>TIR mode:</b> total internal reflection occurs only when n₁ &gt; n₂ and θ₁ exceeds the critical angle θc = asin(n₂/n₁). The chart shows θ₂ vs θ₁ up to θc.</p>"
            "<p><b>Chart:</b> regression of sin θ₂ vs sin θ₁ gives slope n₁/n₂. Error bars show 1σ uncertainty.</p>"
            "<p>Try setting n₁=1.5, n₂=1.0, θ₁=50° to see total internal reflection.</p>");
    });
    auto* refrHelpBar = new QHBoxLayout;
    refrHelpBar->setContentsMargins(0, 0, 0, 0);
    refrHelpBar->addStretch(1);
    refrHelpBar->addWidget(refrHelpBtn);
    m_diagram->body()->addLayout(refrHelpBar);
    m_diagram->body()->addWidget(m_canvasStack);
    m_chart = new LineChart;
    m_chartPanel = new Panel("Regression analysis", "");
    m_chartPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_chartPanel->body()->addWidget(m_chart);
    cols.center->addWidget(m_diagram, 3);
    cols.center->addWidget(m_chartPanel, 2);

    m_results = new Panel("Refraction & TIR calculation", ctx->nextRunLabel());
    m_reg1 = new RegisterView;
    m_reg2 = new RegisterView;
    m_results->body()->addWidget(m_reg1);
    m_results->body()->addWidget(m_reg2);
    m_notice = new NoticeCard;
    cols.right->addWidget(m_results);
    cols.right->addWidget(m_notice);
    cols.right->addStretch();
    setPanels(m_diagram, m_chartPanel, m_results, "LIVE · 1:1", "");

    connect(guidedLab, &QPushButton::clicked, this, [this, ctx] {
        auto* guide = new GuidedLabDialog(ctx, [this] {
            m_mode->setCurrent(0);
            m_med1->setCurrentIndex(m_med1->findData(QStringLiteral("air")));
            m_med2->setCurrentIndex(m_med2->findData(QStringLiteral("crown")));
            m_n1->setValue(1.0, 2);
            m_n2->setValue(1.5, 2);
            m_theta->setValue(30.0, 2);
            m_sigmaN1->setValue(0.0, 3);
            m_sigmaN2->setValue(0.0, 3);
            m_sigmaTheta->setValue(0.0, 2);
            apply(false);
        }, this);
        guide->setAttribute(Qt::WA_DeleteOnClose);
        guide->show();
        guide->raise();
        guide->activateWindow();
    });

    connect(m_med1, &QComboBox::activated, this, [this] { onMediumChosen(m_med1, m_n1); });
    connect(m_med2, &QComboBox::activated, this, [this] { onMediumChosen(m_med2, m_n2); });
    connect(m_n1, &UnitField::edited, this, [this] { onIndexEdited(m_med1, m_n1); });
    connect(m_n2, &UnitField::edited, this, [this] { onIndexEdited(m_med2, m_n2); });
    connect(m_theta, &UnitField::edited, this, [this] { apply(false); });
    for (auto* field : {m_sigmaN1, m_sigmaN2, m_sigmaTheta}) connect(field, &UnitField::edited, this, [this] { apply(false); });
    connect(m_mode, &ModeTabs::currentChanged, this, [this] {
        const bool tir = m_mode->current() == 1;
        if (tir != m_lastModeTir) { m_theta->setValue(tir ? 50.0 : 30.0, 2); m_lastModeTir = tir; }   // the design's default angle per mode
        updateModeTexts();
        apply(false);
    });
    reset();
}

QString RefractionPage::mediumName(QComboBox* combo) const {
    static const std::pair<const char*, const char*> names[] = {{"crown", "Crown glass"}, {"flint", "Flint glass"}, {"acrylic", "Acrylic"},
                                                                {"water", "Water"}, {"diamond", "Diamond"}, {"air", "Air"}};
    const QString id = combo->currentData().toString();
    for (const auto& [key, label] : names) if (id == key) return label;
    return id.isEmpty() ? "Medium" : shortName(combo->currentText());
}

void RefractionPage::updateModeTexts() {
    const bool tir = m_mode->current() == 1;
    m_canvasStack->setCurrentIndex(tir ? 1 : 0);
    m_diagram->setTitle(tir ? "Total internal reflection / ray diagram" : "Ray diagram / two incidence cases");
    m_chartPanel->setTitle(tir ? "Critical-angle sweep" : "Regression analysis");
    m_results->setTitle(tir ? "Results / total internal reflection" : "Results / Snell's law");
    setNormalHint(tir ? "SI units · angles in degrees\n\nTIR requires n₁ > n₂\nand θ₁ > θc." : "SI units · angles in degrees");
}

void RefractionPage::onMediumChosen(QComboBox* combo, UnitField* field) {
    if (const auto* m = m_ctx->presets.findMedium(combo->currentData().toString().toStdString())) field->setValue(m->n);
    apply(false);
}

void RefractionPage::onIndexEdited(QComboBox* combo, UnitField* field) {
    bool ok = false;
    const double v = field->value(&ok);
    const auto* m = m_ctx->presets.findMedium(combo->currentData().toString().toStdString());
    if (ok && (!m || std::fabs(m->n - v) > 1e-9)) combo->setCurrentIndex(combo->count() - 1);    // "Custom"
    apply(false);
}

void RefractionPage::reset() {
    m_lastModeTir = false;
    m_mode->setCurrent(0);
    m_med1->setCurrentIndex(m_med1->findData("crown"));
    m_med2->setCurrentIndex(m_med2->findData("air"));
    m_n1->setValue(1.5);
    m_n2->setValue(1.0);
    m_theta->setValue(30.0, 2);
    m_sigmaN1->setValue(0.0, 4); m_sigmaN2->setValue(0.0, 4); m_sigmaTheta->setValue(0.0, 4);
    m_haveLast = false;
    updateModeTexts();
    apply(false);
}

void RefractionPage::apply(bool record) {
    if (!readFields()) return;
    RefractionInput in;
    in.n1 = m_n1->value(); in.n2 = m_n2->value(); in.theta1 = m_theta->value();
    in.sigmaN1 = m_sigmaN1->value(); in.sigmaN2 = m_sigmaN2->value(); in.sigmaTheta1 = m_sigmaTheta->value();
    in.tirMode = m_mode->current() == 1;
    in.name1 = mediumName(m_med1).toStdString();
    in.name2 = mediumName(m_med2).toStdString();

    const Report r = ER::refraction(in, reportContext());
    showRegisters(m_reg1, m_reg2, r);
    m_notice->setNotice(r.notices[0]);
    applyIssues(r);
    if (r.paused) return;                                         // keep the last valid diagram and chart

    m_last = r;
    m_haveLast = true;
    captureValid();
    m_canvas->setInput(in, shortName(q(in.name1)), shortName(q(in.name2)));
    m_tirCanvas->setInput(in);

    if (in.tirMode) {
        const auto crit = PC::criticalAngleDeg(in.n1, in.n2);
        const auto critError = propagateUncertainty({in.n1, in.n2}, {in.sigmaN1, in.sigmaN2}, [](const std::vector<double>& v) {
            const double ratio = v[1] / v[0];
            return ratio >= 0.0 && ratio < 1.0 ? std::asin(ratio) * 180.0 / std::numbers::pi : kNaN;
        });
        ChartSeries curve;
        curve.line = true;
        const double top = crit ? *crit : 90.0;
        for (int ai = 0; ai * 0.5 <= top; ++ai) {
            const double a = ai * 0.5;
            const double s = in.n1 * std::sin(a * std::numbers::pi / 180.0) / in.n2;
            if (std::fabs(s) <= 1.0) curve.points << QPointF(a, std::asin(s) * 180.0 / std::numbers::pi);
        }
        const Theme& t = ThemeManager::instance().theme();
        QList<ChartVLine> v{{in.theta1, QString::number(in.theta1, 'g', 5) + "° input", t.text, false}};
        QList<ChartRegion> reg;
        QList<ChartPoint> pts;
        if (crit) {
            v.prepend({*crit, QString(" ") + QString::number(*crit, 'f', 4) + "°", t.sage, true});
            reg << ChartRegion{*crit, 90, t.amber, "TIR / no real θ₂"};
            ChartPoint criticalPoint{*crit, 90, "", t.sage, false};
            criticalPoint.xUncertainty = critError.valid ? critError.standardUncertainty : 0.0;
            pts << criticalPoint;
        }
        ChartSeries samples;
        samples.markers = true;
        for (const auto& row : ER::tirSweep(in).rows) if (std::isfinite(row[1])) {
            samples.points << QPointF(row[0], row[1]);
            samples.xUncertainties << in.sigmaTheta1;
            samples.yUncertainties << row[2];
        }
        m_chart->setAxes(0, 90, 3, 0, "θ₁ / degrees", 0, 90, 2, 0, "θ₂ / degrees");
        m_chart->setSeries({curve, samples});
        m_chart->setAnnotations(v, {}, reg, pts);
        m_chart->setLegend({"Samples · error bars = 1σ"}, QString("(") + QString::number(in.n1 / in.n2, 'g', 5) + " sin θ₁), 0 ≤ θ₁ ≤ θc only");
    } else {
        const DataSeries sweep = ER::refractionSweep(in);
        ChartSeries samples, fitLine;
        std::vector<double> xs, ys, ySigmas;
        for (const auto& row : sweep.rows)
            if (std::isfinite(row[1])) {
                xs.push_back(row[2]); ys.push_back(row[3]); ySigmas.push_back(row[6]);
                samples.points << QPointF(row[2], row[3]);
                samples.xUncertainties << row[5]; samples.yUncertainties << row[6];
            }
        samples.markers = true;
        const auto fit = ER::linearFit(xs, ys, ySigmas);
        if (fit.ok) { fitLine.line = true; fitLine.points << QPointF(0, fit.intercept) << QPointF(0.6, fit.slope * 0.6 + fit.intercept); }
        m_chart->clearAnnotations();
        m_chart->setAxes(0, 0.6, 4, 2, "sin θ₁", 0, 1.0, 2, 1, "sin θ₂");
        m_chart->setSeries({fitLine, samples});
        const QString fitCaption = fit.ok ? QString("%1 fit (1σ) · m %2 ± %3 · b %4 ± %5 · R² %6")
            .arg(fit.weighted ? QStringLiteral("Weighted") : QStringLiteral("OLS"))
            .arg(fit.slope, 0, 'f', 4).arg(fit.slopeUncertainty, 0, 'g', 3)
            .arg(fit.intercept, 0, 'f', 4).arg(fit.interceptUncertainty, 0, 'g', 3).arg(fit.r2, 0, 'f', 4) : QString();
        m_chart->setLegend({"Samples · 1σ bars"}, fitCaption);
    }

    Snapshot snap{"Refraction & TIR", r.parameters, r.registers, r.headline, in.tirMode ? ER::tirSweep(in) : ER::refractionSweep(in), {}};
    m_ctx->publish(snap);
    if (record) {
        const QString n1 = shortName(m_med1->currentText()), n2 = shortName(m_med2->currentText());
        const QString desc = m_med1->currentText() == "Custom" || m_med2->currentText() == "Custom"
            ? QString("n₁ %1 → n₂ %2 / %3°").arg(in.n1).arg(in.n2).arg(in.theta1)
            : QString("%1 → %2 / %3°").arg(n1, n2).arg(in.theta1);
        m_ctx->recordRun("Refraction & TIR", (in.tirMode ? "TIR · " : "") + desc, q(r.headline), true);
        refreshMetas(false);
    }
}

// ============================================================================ Lenses & Mirrors
LensesPage::LensesPage(AppContext* ctx, QWidget* parent)
    : ExperimentPage("Lenses & Mirrors", "Gaussian optics · real-is-positive sign convention", "Lenses & Mirrors", ctx, parent) {
    auto cols = addThreeColumns();
    auto* controls = new Panel("Experiment controls");
    controls->body()->setSpacing(10);
    cols.left->addWidget(controls, 1);

    m_mode = new ModeTabs({"Thin lens", "Mirror"});
    m_type = new QComboBox;
    m_f = new UnitField("cm");  m_f->setShowPlus(true);
    m_r = new UnitField("cm");  m_r->setShowPlus(true);
    m_s = new UnitField("cm");  m_s->setShowPlus(true);
    m_h = new UnitField("cm");  m_h->setShowPlus(true);
    m_sigmaF = new UnitField("cm"); m_sigmaS = new UnitField("cm"); m_sigmaH = new UnitField("cm");
    m_sigmaF->setObjectName(FieldIds::sigmaFocalLength);
    m_sigmaS->setObjectName(FieldIds::sigmaObjectDistance);
    m_sigmaH->setObjectName(FieldIds::sigmaObjectHeight);
    bind(QString::fromLatin1(FieldIds::focalLength), m_f); bind(QString::fromLatin1(FieldIds::objectDistance), m_s); bind(QString::fromLatin1(FieldIds::objectHeight), m_h);
    bind(QString::fromLatin1(FieldIds::sigmaFocalLength), m_sigmaF); bind(QString::fromLatin1(FieldIds::sigmaObjectDistance), m_sigmaS);
    bind(QString::fromLatin1(FieldIds::sigmaObjectHeight), m_sigmaH);
    m_typeLabel = new QLabel("Lens type");
    m_typeLabel->setObjectName("fieldLabel");
    auto* typeBox = new QWidget;
    auto* tl = new QVBoxLayout(typeBox);
    tl->setContentsMargins(0, 0, 0, 0);
    tl->setSpacing(5);
    tl->addWidget(m_typeLabel);
    tl->addWidget(m_type);
    m_radiusBlock = labeled("Radius of curvature · R = 2f", m_r);
    auto* f = controls->body();
    f->addWidget(m_mode);
    m_mode->setToolTip("Thin lens: converging/diverging lens imaging.  Mirror: spherical concave/convex mirror imaging.");
    f->addWidget(typeBox);
    m_type->setToolTip("Lens/mirror type sets the sign convention for f and R.");
    f->addWidget(labeled("Focal length · f", m_f));
    m_f->setToolTip("Focal length in cm. Positive = converging lens / concave mirror, negative = diverging.");
    f->addWidget(m_radiusBlock);
    m_r->setToolTip("Radius of curvature R = 2f in cm (mirror mode only).");
    f->addWidget(labeled("Object distance · dₒ", m_s));
    m_s->setToolTip("Object distance from the optical element in cm (real-is-positive convention).");
    f->addWidget(labeled("Object height · hₒ", m_h));
    m_h->setToolTip("Object height in cm. Sign indicates orientation (positive = upright).");
    f->addWidget(uncertaintyInputs({{"σ(f)", m_sigmaF}, {"σ(dₒ)", m_sigmaS}, {"σ(hₒ)", m_sigmaH}}));
    m_sigmaF->setToolTip("Standard uncertainty (1σ) on focal length. Enter 0 to treat as exact.");
    m_sigmaS->setToolTip("Standard uncertainty (1σ) on object distance. Enter 0 to treat as exact.");
    m_sigmaH->setToolTip("Standard uncertainty (1σ) on object height. Enter 0 to treat as exact.");
    // ---- Preset scenarios (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    f->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double fVal, double sVal, double hVal) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        connect(b, &QPushButton::clicked, this, [this, fVal, sVal, hVal] {
            m_syncing = true;
            m_f->setValue(fVal, 2);
            m_s->setValue(sVal, 2);
            m_h->setValue(hVal, 2);
            m_syncing = false;
            apply(false);
        });
        presetRow->addWidget(b);
        return b;
    };
    m_presetBeyond2f = makePreset("dₒ > 2f", 10.0, 30.0, 3.0);
    m_presetAt2f = makePreset("dₒ = 2f", 10.0, 20.0, 3.0);
    m_presetBetweenF2f = makePreset("f < dₒ < 2f", 10.0, 15.0, 3.0);
    m_presetAtF = makePreset("dₒ = f (no image)", 10.0, 10.0, 3.0);
    m_presetInsideF = makePreset("dₒ < f", 10.0, 5.0, 3.0);
    f->addLayout(presetRow);
    buildButtons(f, "Real-is-positive. Distances measured from the optical element.");

    // centre: lens mode = lens + mirror comparison, mirror mode = one detailed mirror + chart
    m_centerStack = new QStackedWidget;
    auto* lensPage = new QWidget;
    auto* lp = new QVBoxLayout(lensPage);
    lp->setContentsMargins(0, 0, 0, 0);
    lp->setSpacing(12);
    m_lens = new LensCanvas;
    m_lensPanel = new Panel("Thin lens / principal rays", "SCALE: cm");
    m_lensPanel->body()->setContentsMargins(0, 0, 0, 0);
    auto* lensHelpBtn = new QToolButton;
    lensHelpBtn->setText("?");
    lensHelpBtn->setFixedSize(24, 24);
    lensHelpBtn->setToolTip("How to use — Lenses & Mirrors");
    lensHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(lensHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Lenses & Mirrors",
            "<h3>Lenses &amp; Mirrors</h3>"
            "<p><b>f:</b> focal length in cm. Positive = converging lens or concave mirror, negative = diverging.</p>"
            "<p><b>dₒ:</b> object distance from the optical element in cm.</p>"
            "<p><b>hₒ:</b> object height in cm.</p>"
            "<p><b>Ray toggles:</b> show/hide the principal rays (parallel, focal, center).</p>"
            "<p><b>Mirror mode:</b> shows a detailed concave mirror with R = 2f and image distance chart.</p>"
            "<p>Presets cover dₒ &gt; 2f, dₒ = 2f, f &lt; dₒ &lt; 2f, dₒ = f, and dₒ &lt; f cases.</p>");
    });
    auto* lensHelpBar = new QHBoxLayout;
    lensHelpBar->setContentsMargins(0, 0, 0, 0);
    lensHelpBar->addStretch(1);
    lensHelpBar->addWidget(lensHelpBtn);
    m_lensPanel->body()->addLayout(lensHelpBar);
    // ---- Principal-ray toggles for lens canvas (Item 18) ----
    auto* lensRayBar = new QWidget;
    auto* lensRayLayout = new QHBoxLayout(lensRayBar);
    lensRayLayout->setContentsMargins(8, 4, 8, 4);
    lensRayLayout->setSpacing(8);
    m_rayParallel = new QCheckBox("Parallel ray");
    m_rayParallel->setChecked(true);
    m_rayParallel->setObjectName("rayToggle");
    m_rayFocal = new QCheckBox("Focal ray");
    m_rayFocal->setChecked(true);
    m_rayFocal->setObjectName("rayToggle");
    m_rayCenter = new QCheckBox("Center ray");
    m_rayCenter->setChecked(true);
    m_rayCenter->setObjectName("rayToggle");
    connect(m_rayParallel, &QCheckBox::toggled, m_lens, [this](bool on) { m_lens->setShowParallelRay(on); });
    connect(m_rayFocal, &QCheckBox::toggled, m_lens, [this](bool on) { m_lens->setShowFocalRay(on); });
    connect(m_rayCenter, &QCheckBox::toggled, m_lens, [this](bool on) { m_lens->setShowCenterRay(on); });
    lensRayLayout->addWidget(m_rayParallel);
    lensRayLayout->addWidget(m_rayFocal);
    lensRayLayout->addWidget(m_rayCenter);
    lensRayLayout->addStretch();
    m_lensPanel->body()->addWidget(lensRayBar);
    m_lensPanel->body()->addWidget(m_lens);
    m_mirrorSmall = new MirrorCanvas;
    m_mirrorSmallPanel = new Panel("Spherical mirror / concave", "");
    m_mirrorSmallPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_mirrorSmallPanel->body()->addWidget(m_mirrorSmall);
    lp->addWidget(m_lensPanel, 1);
    lp->addWidget(m_mirrorSmallPanel, 1);

    auto* mirrorPage = new QWidget;
    auto* mp = new QVBoxLayout(mirrorPage);
    mp->setContentsMargins(0, 0, 0, 0);
    mp->setSpacing(12);
    m_mirrorBig = new MirrorCanvas;
    m_mirrorBig->setDetailed(true);
    // ---- Principal-ray toggles for mirror canvas (Item 18) ----
    auto* mirrorRayBar = new QWidget;
    auto* mirrorRayLayout = new QHBoxLayout(mirrorRayBar);
    mirrorRayLayout->setContentsMargins(8, 4, 8, 4);
    mirrorRayLayout->setSpacing(8);
    auto* mrayParallel = new QCheckBox("Parallel ray");
    mrayParallel->setChecked(true);
    mrayParallel->setObjectName("rayToggle");
    auto* mrayCenter = new QCheckBox("Center ray");
    mrayCenter->setChecked(true);
    mrayCenter->setObjectName("rayToggle");
    connect(mrayParallel, &QCheckBox::toggled, m_mirrorBig, [this](bool on) { m_mirrorBig->setShowParallelRay(on); });
    connect(mrayCenter, &QCheckBox::toggled, m_mirrorBig, [this](bool on) { m_mirrorBig->setShowCenterRay(on); });
    mirrorRayLayout->addWidget(mrayParallel);
    mirrorRayLayout->addWidget(mrayCenter);
    mirrorRayLayout->addStretch();
    m_mirrorBigPanel = new Panel("Spherical mirror / concave", "");
    m_mirrorBigPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_mirrorBigPanel->body()->addWidget(mirrorRayBar);
    m_mirrorBigPanel->body()->addWidget(m_mirrorBig);
    m_chart = new LineChart;
    m_chartPanel = new Panel("Image distance vs object distance", "");
    m_chartPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_chartPanel->body()->addWidget(m_chart);
    mp->addWidget(m_mirrorBigPanel, 3);
    mp->addWidget(m_chartPanel, 2);
    m_centerStack->addWidget(lensPage);
    m_centerStack->addWidget(mirrorPage);
    cols.center->addWidget(m_centerStack, 1);

    m_results = new Panel("Lenses & Mirrors calculation", ctx->nextRunLabel());
    m_reg1 = new RegisterView;
    m_reg2 = new RegisterView;
    m_results->body()->addWidget(m_reg1);
    m_results->body()->addWidget(m_reg2);
    m_notice = new NoticeCard;
    m_caption = new QLabel;
    m_caption->setObjectName("caption");
    m_caption->setWordWrap(true);
    cols.right->addWidget(m_results);
    cols.right->addWidget(m_notice);
    cols.right->addWidget(m_caption);
    cols.right->addStretch();
    setPanels(m_lensPanel, m_chartPanel, m_results, "SCALE: cm", "");

    connect(m_mode, &ModeTabs::currentChanged, this, [this] { updateModeTexts(); apply(false); });
    connect(m_type, &QComboBox::activated, this, [this] { onTypeChosen(); });
    connect(m_f, &UnitField::edited, this, [this] { onFocalEdited(); });
    connect(m_r, &UnitField::edited, this, [this] { onRadiusEdited(); });
    for (auto* field : {m_s, m_h, m_sigmaF, m_sigmaS, m_sigmaH}) connect(field, &UnitField::edited, this, [this] { apply(false); });
    reset();
}

void LensesPage::updateModeTexts() {
    const bool mirror = m_mode->current() == 1;
    const int sign = m_type->currentIndex();
    QSignalBlocker block(m_type);
    m_type->clear();
    m_type->addItems(mirror ? QStringList{"Concave · spherical", "Convex · spherical"} : QStringList{"Converging · biconvex", "Diverging · biconcave"});
    m_type->setCurrentIndex(std::max(0, sign));
    m_typeLabel->setText(mirror ? "Mirror type" : "Lens type");
    m_radiusBlock->setVisible(mirror);
    m_centerStack->setCurrentIndex(mirror ? 1 : 0);
    setPanels(mirror ? m_mirrorBigPanel : m_lensPanel, m_chartPanel, m_results, "SCALE: cm", "");
    setNormalHint(mirror ? "Real-is-positive. For a convex mirror, f and R are negative: the focus and center lie behind the mirror."
                         : "Real-is-positive. Distances measured from the optical element.");
}

void LensesPage::onTypeChosen() {                          // the type decides the sign of f
    bool ok = false;
    const double f = m_f->value(&ok);
    const double magnitude = ok && f != 0 ? std::fabs(f) : 10.0;
    m_syncing = true;
    m_f->setValue(m_type->currentIndex() == 1 ? -magnitude : magnitude, 2);
    m_r->setValue(2 * m_f->value(), 2);
    m_syncing = false;
    apply(false);
}

void LensesPage::onFocalEdited() {                         // the sign of f decides the type; R follows
    if (m_syncing) return;
    bool ok = false;
    const double f = m_f->value(&ok);
    if (ok && f != 0) {
        m_syncing = true;
        m_type->setCurrentIndex(f < 0 ? 1 : 0);
        m_r->setValue(2 * f, 2);
        m_syncing = false;
    }
    apply(false);
}

void LensesPage::onRadiusEdited() {                        // R = 2f
    if (m_syncing) return;
    bool ok = false;
    const double r = m_r->value(&ok);
    if (ok && r != 0) {
        m_syncing = true;
        m_f->setValue(r / 2, 2);
        m_type->setCurrentIndex(r < 0 ? 1 : 0);
        m_syncing = false;
    }
    apply(false);
}

void LensesPage::reset() {
    m_mode->setCurrent(0);
    updateModeTexts();
    m_type->setCurrentIndex(0);
    m_syncing = true;
    m_f->setValue(10.0, 2);
    m_r->setValue(20.0, 2);
    m_s->setValue(30.0, 2);
    m_h->setValue(3.0, 2);
    m_sigmaF->setValue(0.0, 4); m_sigmaS->setValue(0.0, 4); m_sigmaH->setValue(0.0, 4);
    m_syncing = false;
    m_haveLast = false;
    apply(false);
}

void LensesPage::apply(bool record) {
    if (m_syncing || !readFields()) return;
    ImagingInput in;
    in.f = m_f->value(); in.s = m_s->value(); in.h = m_h->value();
    in.sigmaF = m_sigmaF->value(); in.sigmaS = m_sigmaS->value(); in.sigmaH = m_sigmaH->value();
    in.mirrorMode = m_mode->current() == 1;

    const Report r = ER::imaging(in, reportContext());
    showRegisters(m_reg1, m_reg2, r);
    m_notice->setNotice(r.notices[0]);
    m_caption->setText(q(r.footnote));
    m_caption->setVisible(!r.footnote.empty());
    applyIssues(r);
    if (r.paused) return;

    m_last = r;
    m_haveLast = true;
    captureValid();
    if (std::fabs(m_r->value() - 2 * in.f) > 1e-9) { QSignalBlocker b(m_r); m_r->setValue(2 * in.f, 2); }
    m_lens->setInput(in);
    m_mirrorSmall->setInput(in);
    m_mirrorBig->setInput(in);
    const bool concave = in.f > 0;
    m_mirrorSmallPanel->setTitle(concave ? "Spherical mirror / concave" : "Spherical mirror / convex");
    m_mirrorSmallPanel->setMeta(QString("R = 2f = %1 cm").arg(QString::number(2 * in.f, 'g', 6)));
    m_mirrorBigPanel->setTitle(concave ? "Concave spherical mirror / principal rays" : "Convex spherical mirror / principal rays");
    if (in.mirrorMode) {
        StatusInfo info;
        info.mid = q(r.statusText);
        setStatusInfo(info, false);
    }

    if (in.mirrorMode) {                                     // di(do) curve with the current sample
        const double af = std::fabs(in.f);
        QList<ChartSeries> series;
        ChartSeries branch;
        branch.line = true;
        double lo = 1e9, hi = -1e9;
        for (int si = 2; si * 0.25 <= 60.0; ++si) {
            const double so = si * 0.25;
            const auto img = PC::imageFormation(so, in.f);
            if (img.atInfinity || std::fabs(so - in.f) < 0.3 || std::fabs(img.imageDistance) > 4 * af + 1e-9 * 0) {
                if (!branch.points.isEmpty()) { series << branch; branch = ChartSeries(); branch.line = true; }
                continue;
            }
            branch.points << QPointF(so, img.imageDistance);
            lo = std::min(lo, img.imageDistance);
            hi = std::max(hi, img.imageDistance);
        }
        if (!branch.points.isEmpty()) series << branch;
        const double cur = PC::imageFormation(in.s, in.f).imageDistance;
        lo = std::min({lo, cur, in.f < 0 ? in.f : 0.0});
        hi = std::max({hi, cur, in.f < 0 ? 0.0 : in.f});
        double y0, y1;
        niceRange(std::min(lo, 0.0), std::max(hi, 0.0), 2, y0, y1);
        const Theme& t = ThemeManager::instance().theme();
        m_chart->setAxes(0, 60, 4, 0, "dₒ (cm)", y0, y1, 2, 0, "dᵢ / cm");
        m_chart->setSeries(series);
        const auto imageError = propagateUncertainty({in.f, in.s}, {in.sigmaF, in.sigmaS}, [](const std::vector<double>& v) {
            const double denominator = 1.0 / v[0] - 1.0 / v[1];
            return std::fabs(denominator) > 1e-15 ? 1.0 / denominator : kNaN;
        });
        ChartPoint currentPoint{in.s, cur, QString("Current: (%1, %2) cm").arg(signedText(in.s, 0), QString::number(cur, 'g', 4)), t.sage, true};
        currentPoint.xUncertainty = in.sigmaS;
        currentPoint.yUncertainty = imageError.valid ? imageError.standardUncertainty : 0.0;
        m_chart->setAnnotations({{in.s, "", t.dim, false}}, {{in.f, QString(" ") + QString::number(in.f, 'g', 5) + " cm", t.dim}}, {}, {currentPoint});
        const QString fText = QString::number(in.f, 'g', 5);
        m_chart->setLegend({}, QString("▬ dᵢ = %1dₒ/(dₒ %2 %3)   ● Current · error bars = 1σ").arg(fText, in.f < 0 ? "+" : "−").arg(QString::number(af, 'g', 5)));
    }

    m_ctx->publish({"Lenses & Mirrors", r.parameters, r.registers, r.headline, ER::imagingSweep(in), {}});
    if (record) {
        m_ctx->recordRun("Lenses & Mirrors", QString("%1 · f %2 cm").arg(in.mirrorMode ? (concave ? "Concave mirror" : "Convex mirror") : (concave ? "Converging lens" : "Diverging lens"))
                                                 .arg(QString::number(in.f, 'g', 5)), q(r.headline), true);
        refreshMetas(false);
    }
}

// ============================================================================ Prisms
PrismsPage::PrismsPage(AppContext* ctx, QWidget* parent)
    : ExperimentPage("Prisms", "Refraction through a prism · minimum deviation", "Prisms", ctx, parent) {
    auto cols = addThreeColumns();
    auto* controls = new Panel("Experiment controls");
    controls->body()->setSpacing(10);
    cols.left->addWidget(controls, 1);

    m_mode = new ModeTabs({"Single ray", "Dispersion"});
    m_material = mediaCombo(ctx->presets, false);
    m_surround = mediaCombo(ctx->presets, true);
    m_apex = new UnitField("°");
    m_incidence = new UnitField("°");
    m_lambda = new UnitField("nm");
    m_sigmaNPrism = new UnitField; m_sigmaNMedium = new UnitField;
    m_sigmaApex = new UnitField("°"); m_sigmaIncidence = new UnitField("°"); m_sigmaLambda = new UnitField("nm");
    m_sigmaNPrism->setObjectName(FieldIds::sigmaPrismIndex);
    m_sigmaNMedium->setObjectName(FieldIds::sigmaMediumIndex);
    m_sigmaApex->setObjectName(FieldIds::sigmaPrismApex);
    m_sigmaIncidence->setObjectName(FieldIds::sigmaPrismIncidence);
    m_sigmaLambda->setObjectName(FieldIds::sigmaWavelength);
    bind(QString::fromLatin1(FieldIds::prismApex), m_apex); bind(QString::fromLatin1(FieldIds::incidenceAngle), m_incidence); bind(QString::fromLatin1(FieldIds::wavelength), m_lambda);
    bind(QString::fromLatin1(FieldIds::sigmaPrismIndex), m_sigmaNPrism); bind(QString::fromLatin1(FieldIds::sigmaMediumIndex), m_sigmaNMedium);
    bind(QString::fromLatin1(FieldIds::sigmaPrismApex), m_sigmaApex); bind(QString::fromLatin1(FieldIds::sigmaPrismIncidence), m_sigmaIncidence);
    bind(QString::fromLatin1(FieldIds::sigmaWavelength), m_sigmaLambda);
    m_materialLabel = new QLabel;
    m_materialLabel->setObjectName("fieldLabel");
    m_surroundLabel = new QLabel;
    m_surroundLabel->setObjectName("fieldLabel");
    auto holder = [](QLabel* label, QWidget* field) {
        auto* w = new QWidget;
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(5);
        l->addWidget(label);
        l->addWidget(field);
        return w;
    };
    auto* f = controls->body();
    f->addWidget(m_mode);
    m_mode->setToolTip("Single ray: trace one ray through the prism.  Dispersion: show how different wavelengths bend differently.");
    f->addWidget(holder(m_materialLabel, m_material));
    m_material->setToolTip("Prism material preset (uses Cauchy dispersion if available).");
    f->addWidget(holder(m_surroundLabel, m_surround));
    m_surround->setToolTip("Surrounding medium preset (usually air).");
    f->addWidget(labeled("Apex angle · A", m_apex));
    m_apex->setToolTip("Apex angle of the prism in degrees (typically 60° for equilateral).");
    f->addWidget(labeled("Incidence angle · i₁", m_incidence));
    m_incidence->setToolTip("Angle of incidence on the first face, measured from the normal, in degrees.");
    f->addWidget(labeled("Wavelength · λ", m_lambda));
    m_lambda->setToolTip("Wavelength in nm. Used for Cauchy dispersion (e.g. 589.3 nm sodium D line).");
    f->addWidget(uncertaintyInputs({{"σ(n prism)", m_sigmaNPrism}, {"σ(n medium)", m_sigmaNMedium},
                                    {"σ(A)", m_sigmaApex}, {"σ(i₁)", m_sigmaIncidence}, {"σ(λ)", m_sigmaLambda}}));
    m_sigmaNPrism->setToolTip("Standard uncertainty (1σ) on prism index. Enter 0 to treat as exact.");
    m_sigmaNMedium->setToolTip("Standard uncertainty (1σ) on medium index. Enter 0 to treat as exact.");
    m_sigmaApex->setToolTip("Standard uncertainty (1σ) on apex angle. Enter 0 to treat as exact.");
    m_sigmaIncidence->setToolTip("Standard uncertainty (1σ) on incidence angle. Enter 0 to treat as exact.");
    m_sigmaLambda->setToolTip("Standard uncertainty (1σ) on wavelength. Enter 0 to treat as exact.");
    // ---- Preset scenario buttons (Item 19) ----
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    f->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, double apex, double incidence, const QString& material, const QString& surround) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setMinimumHeight(30);
        connect(b, &QPushButton::clicked, this, [this, apex, incidence, material, surround] {
            m_syncing = true;
            applyPreset(apex, incidence, material, surround);
            m_syncing = false;
            apply(false);
        });
        presetRow->addWidget(b);
        return b;
    };
    m_presetEquilateral = makePreset("Equilateral · crown", 60.0, 48.5904, "crown", "air");
    m_presetMinDev = makePreset("Min dev finder · 60°", 60.0, 30.0, "crown", "air");
    m_presetThin = makePreset("Thin prism · A=10°", 10.0, 5.0, "crown", "air");
    m_presetHighIndex = makePreset("High index · flint", 60.0, 45.0, "flint", "air");
    f->addLayout(presetRow);
    // ---- Minimum-deviation finder (Item 17) ----
    m_btnMinDev = new QPushButton("Set i → i_min");
    m_btnMinDev->setObjectName("primary");
    m_btnMinDev->setCursor(Qt::PointingHandCursor);
    m_btnMinDev->setMinimumHeight(34);
    f->addWidget(m_btnMinDev);
    buildButtons(f, "SI units · angles in degrees");

    m_canvas = new PrismCanvas;
    // ---- Normal and bend toggles (Item 21) ----
    m_chkNormals = new QCheckBox("Show normals");
    m_chkNormals->setChecked(true);
    m_chkNormals->setObjectName("rayToggle");
    m_chkBends = new QCheckBox("Show bend angles");
    m_chkBends->setChecked(true);
    m_chkBends->setObjectName("rayToggle");
    auto* toggleBar = new QWidget;
    auto* toggleLayout = new QHBoxLayout(toggleBar);
    toggleLayout->setContentsMargins(8, 4, 8, 4);
    toggleLayout->setSpacing(8);
    toggleLayout->addWidget(m_chkNormals);
    toggleLayout->addWidget(m_chkBends);
    toggleLayout->addStretch();
    m_diagram = new Panel("Prism ray path", "SYMMETRIC PASSAGE");
    m_diagram->body()->setContentsMargins(0, 0, 0, 0);
    auto* prismHelpBtn = new QToolButton;
    prismHelpBtn->setText("?");
    prismHelpBtn->setFixedSize(24, 24);
    prismHelpBtn->setToolTip("How to use — Prisms");
    prismHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(prismHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Prisms",
            "<h3>Prisms</h3>"
            "<p><b>A:</b> apex angle of the prism in degrees.</p>"
            "<p><b>i₁:</b> angle of incidence on the first face from the normal, in degrees.</p>"
            "<p><b>λ:</b> wavelength in nm (for dispersion calculations).</p>"
            "<p><b>Minimum deviation:</b> click \"Set i → i_min\" to find the angle of minimum deviation, where the ray passes symmetrically through the prism.</p>"
            "<p><b>Dispersion mode:</b> shows how different colours (wavelengths) separate due to wavelength-dependent refractive index.</p>"
            "<p>Presets include equilateral crown (60°), minimum deviation finder, thin prism (10°), and high-index flint.</p>");
    });
    auto* prismHelpBar = new QHBoxLayout;
    prismHelpBar->setContentsMargins(0, 0, 0, 0);
    prismHelpBar->addStretch(1);
    prismHelpBar->addWidget(prismHelpBtn);
    m_diagram->body()->addLayout(prismHelpBar);
    m_diagram->body()->addWidget(toggleBar);
    m_diagram->body()->addWidget(m_canvas);
    m_chart = new LineChart;
    m_chartPanel = new Panel("Deviation vs. incidence angle", "");
    m_chartPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_chartPanel->body()->addWidget(m_chart);
    cols.center->addWidget(m_diagram, 3);
    cols.center->addWidget(m_chartPanel, 2);

    m_results = new Panel("Prisms calculation", ctx->nextRunLabel());
    m_reg = new RegisterView;
    m_noticeInline = new NoticeCard;
    m_footnote = new QLabel;
    m_footnote->setObjectName("mono");
    m_footnote->setWordWrap(true);
    m_results->body()->addWidget(m_reg);
    m_results->body()->addWidget(m_noticeInline);
    m_results->body()->addWidget(m_footnote);
    m_noticeExit = new NoticeCard;
    cols.right->addWidget(m_results);
    cols.right->addWidget(m_noticeExit);
    cols.right->addStretch();
    setPanels(m_diagram, m_chartPanel, m_results, "SYMMETRIC PASSAGE", "");

    connect(m_mode, &ModeTabs::currentChanged, this, [this] { apply(false); });
    connect(m_material, &QComboBox::activated, this, [this] { apply(false); });
    connect(m_surround, &QComboBox::activated, this, [this] { apply(false); });
    for (auto* field : {m_apex, m_incidence, m_lambda, m_sigmaNPrism, m_sigmaNMedium, m_sigmaApex, m_sigmaIncidence, m_sigmaLambda})
        connect(field, &UnitField::edited, this, [this] { apply(false); });
    connect(m_btnMinDev, &QPushButton::clicked, this, &PrismsPage::setMinimumDeviation);
    connect(m_chkNormals, &QCheckBox::toggled, m_canvas, &PrismCanvas::setShowNormals);
    connect(m_chkBends, &QCheckBox::toggled, m_canvas, &PrismCanvas::setShowBends);
    reset();
}

double PrismsPage::indexOf(QComboBox* combo, double lambda) const {
    const auto* m = m_ctx->presets.findMedium(combo->currentData().toString().toStdString());
    if (!m) return 1.5;
    return (m->cauchyA != 0.0 || m->cauchyB != 0.0) ? PC::cauchyIndex(m->cauchyA, m->cauchyB, lambda) : m->n;
}

void PrismsPage::applyPreset(double apex, double incidence, const QString& material, const QString& surround) {
    m_apex->setValue(apex, 2);
    m_incidence->setValue(incidence, 4);
    for (int i = 0; i < m_material->count(); ++i) {
        if (m_material->itemData(i).toString() == material) { m_material->setCurrentIndex(i); break; }
    }
    for (int i = 0; i < m_surround->count(); ++i) {
        if (m_surround->itemData(i).toString() == surround) { m_surround->setCurrentIndex(i); break; }
    }
}

void PrismsPage::setMinimumDeviation() {
    const double nPrism = indexOf(m_material, m_lambda->value());
    const double nMedium = indexOf(m_surround, m_lambda->value());
    const auto imin = PC::minimumDeviationIncidenceDeg(nPrism, nMedium, m_apex->value());
    if (!imin) return;
    m_syncing = true;
    m_incidence->setValue(*imin, 4);
    m_syncing = false;
    apply(false);
}

void PrismsPage::reset() {
    m_mode->setCurrent(0);
    m_material->setCurrentIndex(m_material->findData("crown"));
    m_surround->setCurrentIndex(m_surround->findData("air"));
    m_apex->setValue(60.0, 2);
    m_incidence->setValue(48.5904, 4);
    m_lambda->setValue(589.3, 1);
    m_sigmaNPrism->setValue(0.0, 5); m_sigmaNMedium->setValue(0.0, 5);
    m_sigmaApex->setValue(0.0, 4); m_sigmaIncidence->setValue(0.0, 4); m_sigmaLambda->setValue(0.0, 4);
    m_haveLast = false;
    apply(false);
}

void PrismsPage::apply(bool record) {
    if (m_syncing || !readFields()) return;
    PrismInput in;
    in.apex = m_apex->value(); in.incidence = m_incidence->value(); in.wavelengthNm = m_lambda->value();
    in.sigmaNPrism = m_sigmaNPrism->value(); in.sigmaNMedium = m_sigmaNMedium->value();
    in.sigmaApex = m_sigmaApex->value(); in.sigmaIncidence = m_sigmaIncidence->value(); in.sigmaWavelengthNm = m_sigmaLambda->value();
    in.dispersion = m_mode->current() == 1;
    const double lambda = std::max(1.0, in.wavelengthNm);
    in.nPrism = indexOf(m_material, lambda);
    in.nMedium = indexOf(m_surround, lambda);
    auto wavelengthSlope = [this, lambda](QComboBox* combo) {
        const auto* medium = m_ctx->presets.findMedium(combo->currentData().toString().toStdString());
        if (!medium || medium->cauchyB == 0.0) return 0.0;
        return -2.0 * medium->cauchyB * 1e6 / (lambda * lambda * lambda);
    };
    in.dnPrismDlambda = wavelengthSlope(m_material);
    in.dnMediumDlambda = wavelengthSlope(m_surround);
    m_materialLabel->setText(QString("Prism material · n = %1").arg(in.nPrism, 0, 'f', 3));
    m_surroundLabel->setText(QString("Surrounding medium · n = %1").arg(in.nMedium, 0, 'f', 3));
    m_diagram->setTitle(in.dispersion ? "Prism ray path · visible spectrum" : "Prism ray path");
    setPanels(m_diagram, m_chartPanel, m_results,
              in.dispersion ? "7 COLOURS · 400–700 NM" : "SYMMETRIC PASSAGE", "");

    const Report r = ER::prism(in, reportContext());
    m_reg->setRegister(r.registers[0]);
    if (r.notices.size() >= 2) { m_noticeInline->setNotice(r.notices[0]); m_noticeInline->show(); m_noticeExit->setNotice(r.notices[1]); }
    else { m_noticeInline->hide(); m_noticeExit->setNotice(r.notices[0]); }
    m_footnote->setText(q(r.footnote));
    m_footnote->setVisible(!r.footnote.empty());
    applyIssues(r);
    if (r.paused) return;

    m_last = r;
    m_haveLast = true;
    captureValid();
    QList<PrismPath> extra;
    if (in.dispersion) {
        struct SpectrumSample { double wavelengthNm; const char* color; };
        static constexpr SpectrumSample spectrum[] = {
            {700.0, "#e53935"}, // red
            {620.0, "#fb8c00"}, // orange
            {580.0, "#fdd835"}, // yellow
            {540.0, "#43a047"}, // green
            {490.0, "#1e88e5"}, // blue
            {450.0, "#3f51b5"}, // indigo
            {400.0, "#8e24aa"}  // violet
        };
        extra.reserve(7);
        for (const SpectrumSample& sample : spectrum) {
            extra.append({QColor(sample.color), sample.wavelengthNm,
                          indexOf(m_material, sample.wavelengthNm),
                          indexOf(m_surround, sample.wavelengthNm)});
        }
    }
    m_canvas->setInput(in, extra);

    ChartSeries curve;
    double lo = 1e9, hi = -1e9;
    for (int ai = 70; ai * 0.5 <= 65.0; ++ai) {
        const double a = ai * 0.5;
        const auto p = PC::prism(in.nPrism, in.nMedium, in.apex, a);
        if (!p.valid || p.entryTir || p.exitTir) continue;
        curve.points << QPointF(a, p.deviation);
        lo = std::min(lo, p.deviation);
        hi = std::max(hi, p.deviation);
    }
    curve.line = true;
    ChartSeries samples;
    samples.markers = true;
    for (const auto& row : ER::prismSweep(in).rows) if (std::isfinite(row[1])) {
        samples.points << QPointF(row[0], row[1]);
        samples.xUncertainties << row[2]; samples.yUncertainties << row[3];
    }
    if (!curve.points.isEmpty()) {
        int y0 = int(std::floor(lo)) - 1, y1 = int(std::ceil(hi)) + 1;
        y1 = y0 + static_cast<int>(std::ceil((y1 - y0) / 4.0)) * 4;
        m_chart->setAxes(35, 65, 4, 1, "i₁ (°)", y0, y1, 2, 0, "δ (°)");
    }
    m_chart->setSeries({curve, samples});
    const auto imin = PC::minimumDeviationIncidenceDeg(in.nPrism, in.nMedium, in.apex);
    m_chart->setMarkerX(imin.value_or(0), imin.has_value());
    m_chart->setLegend({"Sweep samples · error bars = 1σ"}, imin ? QString("Ray-traced deviation · minimum at %1°").arg(*imin, 0, 'f', 2) : "");

    m_ctx->publish({"Prisms", r.parameters, r.registers, r.headline, ER::prismSweep(in), {}});
    if (record) {
        m_ctx->recordRun("Prisms", QString("Prism A %1° · i₁ %2°").arg(in.apex).arg(in.incidence, 0, 'f', 2), q(r.headline), true);
        refreshMetas(false);
    }
}

// ============================================================================ Wave interference
WavePage::WavePage(AppContext* ctx, QWidget* parent)
    : ExperimentPage("Wave Interference", "Young's double-slit experiment · coherent monochromatic light", "Wave Interference", ctx, parent) {
    setCompactLayout();
    auto cols = addThreeColumns(232, 250);
    auto* row = qobject_cast<QHBoxLayout*>(content()->itemAt(content()->count() - 1)->layout());
    if (row) row->setSpacing(8);
    for (QVBoxLayout* column : {cols.left, cols.center, cols.right}) column->setSpacing(8);
    auto* controls = new Panel("Experiment controls");
    controls->body()->setContentsMargins(8, 8, 8, 8);
    controls->body()->setSpacing(6);
    cols.left->addWidget(controls, 1);

    m_source = new QComboBox;
    for (const auto& s : ctx->presets.sources) m_source->addItem(q(s.name), s.wavelengthNm);
    m_source->addItem("Custom", -1.0);
    m_lambda = new UnitField("nm");
    m_d = new UnitField("mm");
    m_L = new UnitField("m");
    m_order = new UnitField;  m_order->setSpinner(true, 1.0);
    m_sigmaLambda = new UnitField("nm"); m_sigmaD = new UnitField("mm"); m_sigmaL = new UnitField("m");
    m_sigmaLambda->setObjectName(FieldIds::sigmaWavelength);
    m_sigmaD->setObjectName(FieldIds::sigmaSlitSeparation);
    m_sigmaL->setObjectName(FieldIds::sigmaScreenDistance);
    bind(QString::fromLatin1(FieldIds::wavelength), m_lambda); bind(QString::fromLatin1(FieldIds::slitSeparation), m_d); bind(QString::fromLatin1(FieldIds::screenDistance), m_L); bind(QString::fromLatin1(FieldIds::order), m_order);
    bind(QString::fromLatin1(FieldIds::sigmaWavelength), m_sigmaLambda); bind(QString::fromLatin1(FieldIds::sigmaSlitSeparation), m_sigmaD);
    bind(QString::fromLatin1(FieldIds::sigmaScreenDistance), m_sigmaL);

    m_slitWidth = new UnitField("mm");  m_slitWidth->setSpinner(true, 0.01);
    m_slitWidth->setObjectName(FieldIds::slitWidth);
    bind(QString::fromLatin1(FieldIds::slitWidth), m_slitWidth);
    m_intensityRatio = new UnitField;  m_intensityRatio->setSpinner(true, 0.01);
    m_intensityRatio->setObjectName(FieldIds::intensityRatio);
    for (UnitField* field : {m_lambda, m_d, m_L, m_order, m_sigmaLambda, m_sigmaD, m_sigmaL, m_slitWidth, m_intensityRatio})
        field->setPasteButtonVisible(false);
    bind(QString::fromLatin1(FieldIds::intensityRatio), m_intensityRatio);
    m_chkExact = new QCheckBox("Exact formula y = L tan(asin(mλ/d))");  m_chkExact->setObjectName("rayToggle");
    m_chkEnvelope = new QCheckBox("Show diffraction envelope");  m_chkEnvelope->setObjectName("rayToggle");

    m_view = new ModeTabs({"Intensity", "Wavefronts"});
    auto* f = controls->body();
    f->addWidget(labeled("Source preset", m_source));
    m_source->setToolTip("Choose a standard light source preset (wavelength), or Custom to enter λ manually.");
    f->addWidget(labeled("Wavelength · λ", m_lambda));
    m_lambda->setToolTip("Wavelength of the light in nm (e.g. 532 nm for a green laser).");
    f->addWidget(labeled("Slit separation · d", m_d));
    m_d->setToolTip("Distance between the two slits in mm. Fringe spacing ∝ λL/d.");
    f->addWidget(labeled("Screen distance · L", m_L));
    m_L->setToolTip("Distance from the slits to the screen in m. Fringe spacing ∝ L.");
    f->addWidget(labeled("Fringe order · m", m_order));
    m_order->setToolTip("Integer fringe order. Central maximum is m=0; maxima at mλ/d, minima at (m+½)λ/d.");
    f->addWidget(labeled("Slit width · a (envelope)", m_slitWidth));
    m_slitWidth->setToolTip("Finite slit width in mm (0 = ideal slits). Creates a diffraction envelope that modulates the interference fringes.");
    f->addWidget(labeled("Intensity ratio I₂/I₁", m_intensityRatio));
    m_intensityRatio->setToolTip("Relative intensity of the second slit compared to the first (1.0 = equal intensities).");
    f->addWidget(uncertaintyInputs({{"σ(λ)", m_sigmaLambda}, {"σ(d)", m_sigmaD}, {"σ(L)", m_sigmaL}}));
    m_sigmaLambda->setToolTip("Standard uncertainty (1σ) on wavelength. Enter 0 to treat as exact.");
    m_sigmaD->setToolTip("Standard uncertainty (1σ) on slit separation. Enter 0 to treat as exact.");
    m_sigmaL->setToolTip("Standard uncertainty (1σ) on screen distance. Enter 0 to treat as exact.");
    f->addWidget(m_chkExact);
    m_chkExact->setToolTip("Use the exact formula y = L·tan(asin(mλ/d)) instead of the small-angle approximation y ≈ L·mλ/d.");
    f->addWidget(m_chkEnvelope);
    m_chkEnvelope->setToolTip("Show the single-slit diffraction envelope that modulates the double-slit interference pattern.");
    f->addWidget(m_view);
    m_view->setToolTip("Intensity: show the screen intensity profile.  Wavefronts: show the Huygens wavefront schematic.");
    buildButtons(f, "Small angles: y ≈ Lθ. Ideal narrow slits.");

    // ---- Preset scenarios ---
    auto* presetLabel = new QLabel("Presets:");
    presetLabel->setObjectName("fieldLabel");
    f->addWidget(presetLabel);
    auto* presetRow = new QHBoxLayout;
    presetRow->setSpacing(4);
    auto makePreset = [&](const QString& label, const QString& detail, double lambda, double d, double L, double slitWidth) {
        auto* b = new QPushButton(label);
        b->setObjectName("presetBtn");
        b->setCursor(Qt::PointingHandCursor);
        b->setToolTip(detail);
        b->setMinimumWidth(0);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        b->setMinimumHeight(30);
        connect(b, &QPushButton::clicked, this, [this, lambda, d, L, slitWidth] {
            m_lambda->setValue(lambda, 1);
            m_d->setValue(d, 3);
            m_L->setValue(L, 3);
            m_slitWidth->setValue(slitWidth, 4);
            apply(false);
        });
        presetRow->addWidget(b);
        return b;
    };
    (void)makePreset("Red laser", "650 nm · d = 0.25 mm · L = 1.5 m", 650.0, 0.25, 1.5, 0.0);
    (void)makePreset("Green laser", "532 nm · d = 0.1 mm · L = 2.0 m", 532.0, 0.1, 2.0, 0.0);
    (void)makePreset("Envelope", "532 nm · d = 0.2 mm · L = 1.5 m · a = 0.05 mm", 532.0, 0.2, 1.5, 0.05);
    presetRow->addStretch(1);
    f->addLayout(presetRow);

    m_canvas = new WaveCanvas;
    m_diagram = new Panel("Wavefronts and interference pattern", "SCHEMATIC · NOT TO SCALE");
    m_diagram->body()->setContentsMargins(0, 0, 0, 0);
    auto* waveHelpBtn = new QToolButton;
    waveHelpBtn->setText("?");
    waveHelpBtn->setFixedSize(24, 24);
    waveHelpBtn->setToolTip("How to use — Wave Interference");
    waveHelpBtn->setCursor(Qt::PointingHandCursor);
    connect(waveHelpBtn, &QToolButton::clicked, this, [this] {
        phy104::showHelpDialog(this, "How to use — Wave Interference",
            "<h3>Wave Interference (Double-Slit)</h3>"
            "<p><b>λ:</b> wavelength of light in nm.</p>"
            "<p><b>d:</b> slit separation in mm. Fringe spacing on screen ≈ λL/d.</p>"
            "<p><b>L:</b> screen distance in m.</p>"
            "<p><b>m:</b> fringe order. Maxima at y = m·λL/d, minima at y = (m+½)·λL/d.</p>"
            "<p><b>a (slit width):</b> finite width creates a diffraction envelope that modulates the fringes.</p>"
            "<p><b>I₂/I₁:</b> intensity ratio between slits. Unequal intensities reduce fringe contrast.</p>"
            "<p><b>Exact formula:</b> use y = L·tan(asin(mλ/d)) for large angles instead of the small-angle approximation.</p>");
    });
    auto* waveHelpBar = new QHBoxLayout;
    waveHelpBar->setContentsMargins(0, 0, 0, 0);
    waveHelpBar->addStretch(1);
    waveHelpBar->addWidget(waveHelpBtn);
    m_diagram->body()->addLayout(waveHelpBar);
    m_diagram->body()->addWidget(m_canvas);
    m_chart = new LineChart;
    m_chartPanel = new Panel("Screen intensity profile", "");
    m_chartPanel->body()->setContentsMargins(0, 0, 0, 0);
    m_chartPanel->body()->addWidget(m_chart);
    cols.center->addWidget(m_diagram, 3);
    cols.center->addWidget(m_chartPanel, 2);

    m_results = new Panel("Fringe calculation", ctx->nextRunLabel());
    m_results->body()->setContentsMargins(8, 8, 8, 8);
    m_results->body()->setSpacing(8);
    m_reg1 = new RegisterView;
    m_reg2 = new RegisterView;
    m_results->body()->addWidget(m_reg1);
    m_results->body()->addWidget(m_reg2);
    m_notice = new NoticeCard;
    cols.right->addWidget(m_results);
    cols.right->addWidget(m_notice);
    cols.right->addStretch();
    setPanels(m_diagram, m_chartPanel, m_results, "SCHEMATIC · NOT TO SCALE", "");

    connect(m_source, &QComboBox::activated, this, [this] { onPresetChosen(); });
    connect(m_lambda, &UnitField::edited, this, [this] { onWavelengthEdited(); });
    for (auto* field : {m_d, m_L, m_order, m_sigmaLambda, m_sigmaD, m_sigmaL, m_slitWidth, m_intensityRatio})
        connect(field, &UnitField::edited, this, [this] { apply(false); });
    connect(m_view, &ModeTabs::currentChanged, this, [this] { apply(false); });
    connect(m_chkExact, &QCheckBox::toggled, this, [this] { apply(false); });
    connect(m_chkEnvelope, &QCheckBox::toggled, this, [this] { apply(false); });
    reset();
}

void WavePage::onPresetChosen() {
    const double w = m_source->currentData().toDouble();
    if (w > 0) m_lambda->setValue(w, 1);
    apply(false);
}

void WavePage::onWavelengthEdited() {
    bool ok = false;
    const double v = m_lambda->value(&ok);
    if (ok && std::fabs(m_source->currentData().toDouble() - v) > 1e-9) m_source->setCurrentIndex(m_source->count() - 1);
    apply(false);
}

void WavePage::reset() {
    m_source->setCurrentIndex(0);
    m_lambda->setValue(532.0, 1);
    m_d->setValue(0.25, 3);
    m_L->setValue(1.5, 3);
    m_order->setValue(2, 0);
    m_sigmaLambda->setValue(0.0, 4); m_sigmaD->setValue(0.0, 4); m_sigmaL->setValue(0.0, 4);
    m_slitWidth->setValue(0.0, 4);
    m_intensityRatio->setValue(1.0, 3);
    m_chkExact->setChecked(false);
    m_chkEnvelope->setChecked(false);
    m_view->setCurrent(0);
    m_haveLast = false;
    apply(false);
}

void WavePage::apply(bool record) {
    if (!readFields()) return;
    SlitInput in;
    in.wavelengthNm = m_lambda->value(); in.separationMm = m_d->value(); in.screenM = m_L->value();
    in.sigmaWavelengthNm = m_sigmaLambda->value(); in.sigmaSeparationMm = m_sigmaD->value(); in.sigmaScreenM = m_sigmaL->value();
    in.order = static_cast<int>(std::lround(m_order->value()));
    in.slitWidthMm = m_slitWidth->value();
    in.intensityRatio = m_intensityRatio->value();
    in.useExactFormula = m_chkExact->isChecked();
    in.showEnvelope = m_chkEnvelope->isChecked();

    const Report r = ER::doubleSlit(in, reportContext());
    showRegisters(m_reg1, m_reg2, r);
    m_notice->setNotice(r.notices[0]);
    applyIssues(r);
    if (r.paused) return;                                    // the diagram and chart keep showing the last valid run

    m_last = r;
    m_haveLast = true;
    captureValid();
    m_canvas->setInput(in, m_view->current() == 1);

    ChartSeries curve;
    const double lam = in.wavelengthNm * 1e-9, d = in.separationMm * 1e-3;
    const double slitWidthM = in.slitWidthMm * 1e-3;
    for (int k = -200; k <= 200; ++k) {
        const double y = k * 0.05;                           // mm
        const double env = in.showEnvelope && slitWidthM > 0.0 ? PC::singleSlitEnvelope(y * 1e-3, lam, in.screenM, slitWidthM) : 1.0;
        curve.points << QPointF(y, PC::relativeIntensityUnequal(y * 1e-3, lam, in.screenM, d, in.intensityRatio) * env);
    }
    curve.line = true;
    ChartSeries errorSamples;
    errorSamples.markers = true;
    for (const auto& row : ER::doubleSlitSweep(in).rows) if (std::isfinite(row[1])) {
        errorSamples.points << QPointF(row[0], row[1]);
        errorSamples.xUncertainties << 0.0; errorSamples.yUncertainties << row[2];
    }
    QList<ChartSeries> seriesList = {curve, errorSamples};
    if (in.showEnvelope && slitWidthM > 0.0) {
        ChartSeries envelope;
        envelope.line = true;
        for (int k = -200; k <= 200; ++k) {
            const double y = k * 0.05;
            envelope.points << QPointF(y, PC::singleSlitEnvelope(y * 1e-3, lam, in.screenM, slitWidthM));
        }
        seriesList << envelope;
    }
    m_chart->setAxes(-10, 10, 4, 0, "y (mm)", 0, 1.0, 2, 1, "I / I₀");
    m_chart->setSeries(seriesList);
    QString legendFormula = in.useExactFormula ? QString("exact y = L tan(asin(mλ/d))") : QString("cos²(πdy / λL)");
    QString legendEnvelope = in.showEnvelope && slitWidthM > 0.0 ? QString(" · envelope a = %1 mm").arg(QString::number(in.slitWidthMm, 'f', 3)) : QString();
    QString legendRatio = in.intensityRatio != 1.0 ? QString(" · I₂/I₁ = %1").arg(QString::number(in.intensityRatio, 'f', 2)) : QString();
    m_chart->setLegend({"Samples · error bars = 1σ"}, legendFormula + legendEnvelope + legendRatio + QString(" · d = %1 mm").arg(QString::number(in.separationMm, 'f', 3)));

    m_ctx->publish({"Wave Interference", r.parameters, r.registers, r.headline, ER::doubleSlitSweep(in), {}});
    if (record) {
        m_ctx->recordRun("Wave Interference", QString("Double-slit · %1 nm · d %2 mm").arg(in.wavelengthNm).arg(in.separationMm), q(r.headline), true);
        refreshMetas(false);
    }
}

} // namespace phy104
