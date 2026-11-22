#pragma once
#include "ExperimentBase.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;

namespace phy104 {

class AppContext;
class RegisterView;
class LensCanvas;
class LineChart;
class MirrorCanvas;
class ModeTabs;
class NoticeCard;
class Panel;
class PrismCanvas;
class RefractionCanvas;
class TirCanvas;
class UnitField;
class WaveCanvas;

/// Thin forwarding layer kept for ReportPage compatibility (QList<ExperimentPage*>).
/// All shared experiment logic lives in ExperimentBase; ExperimentPage adds nothing
/// beyond the constructor and remains abstract (apply()/reset() are pure virtual).
class ExperimentPage : public ExperimentBase {
    Q_OBJECT
public:
    ExperimentPage(const QString& title, const QString& subtitle, const QString& experiment,
                   AppContext* context, QWidget* parent = nullptr)
        : ExperimentBase(title, subtitle, experiment, context, parent) {}
};

/// Experiment 01: Snell's law (two cases) and the single-case total internal reflection screen.
class RefractionPage : public ExperimentPage {
    Q_OBJECT
public:
    explicit RefractionPage(AppContext* context, QWidget* parent = nullptr);
protected:
    void apply(bool record) override;
    void reset() override;
private:
    void onMediumChosen(QComboBox* combo, UnitField* field);
    void onIndexEdited(QComboBox* combo, UnitField* field);
    QString mediumName(QComboBox* combo) const;
    void updateModeTexts();

    ModeTabs* m_mode;
    QComboBox *m_med1, *m_med2;
    UnitField *m_n1, *m_n2, *m_theta;
    UnitField *m_sigmaN1, *m_sigmaN2, *m_sigmaTheta;
    QStackedWidget* m_canvasStack;
    RefractionCanvas* m_canvas;
    TirCanvas* m_tirCanvas;
    LineChart* m_chart;
    Panel *m_diagram, *m_chartPanel, *m_results;
    RegisterView *m_reg1, *m_reg2;
    NoticeCard* m_notice;
    bool m_lastModeTir = false;
};

/// Experiment 02: thin lens (two diagrams) and spherical mirror (one detailed diagram + chart).
class LensesPage : public ExperimentPage {
    Q_OBJECT
public:
    explicit LensesPage(AppContext* context, QWidget* parent = nullptr);
protected:
    void apply(bool record) override;
    void reset() override;
private:
    void updateModeTexts();
    void onTypeChosen();
    void onFocalEdited();
    void onRadiusEdited();

    ModeTabs* m_mode;
    QComboBox* m_type;
    QLabel* m_typeLabel;
    QWidget* m_radiusBlock;
    UnitField *m_f, *m_r, *m_s, *m_h;
    UnitField *m_sigmaF, *m_sigmaS, *m_sigmaH;
    QStackedWidget* m_centerStack;
    LensCanvas* m_lens;
    MirrorCanvas *m_mirrorSmall, *m_mirrorBig;
    LineChart* m_chart;
    Panel *m_lensPanel, *m_mirrorSmallPanel, *m_mirrorBigPanel, *m_chartPanel, *m_results;
    RegisterView *m_reg1, *m_reg2;
    NoticeCard* m_notice;
    QLabel* m_caption;
    bool m_syncing = false;
    // ---- Principal-ray toggles (Item 18) ----
    QCheckBox* m_rayParallel = nullptr;
    QCheckBox* m_rayFocal = nullptr;
    QCheckBox* m_rayCenter = nullptr;
    // ---- Preset scenario buttons (Item 19) ----
    QPushButton* m_presetBeyond2f = nullptr;
    QPushButton* m_presetAt2f = nullptr;
    QPushButton* m_presetBetweenF2f = nullptr;
    QPushButton* m_presetAtF = nullptr;
    QPushButton* m_presetInsideF = nullptr;
};

/// Experiment 03: refraction through a prism, minimum deviation, dispersion.
class PrismsPage : public ExperimentPage {
    Q_OBJECT
public:
    explicit PrismsPage(AppContext* context, QWidget* parent = nullptr);
protected:
    void apply(bool record) override;
    void reset() override;
private:
    double indexOf(QComboBox* combo, double wavelengthNm) const;
    void setMinimumDeviation();
    void applyPreset(double apex, double incidence, const QString& material, const QString& surround);

    ModeTabs* m_mode;
    QComboBox *m_material, *m_surround;
    QLabel *m_materialLabel, *m_surroundLabel;
    UnitField *m_apex, *m_incidence, *m_lambda;
    UnitField *m_sigmaNPrism, *m_sigmaNMedium, *m_sigmaApex, *m_sigmaIncidence, *m_sigmaLambda;
    QPushButton *m_presetEquilateral, *m_presetMinDev, *m_presetThin, *m_presetHighIndex;
    QPushButton* m_btnMinDev;
    QCheckBox* m_chkNormals;
    QCheckBox* m_chkBends;
    bool m_syncing = false;
    PrismCanvas* m_canvas;
    LineChart* m_chart;
    Panel *m_diagram, *m_chartPanel, *m_results;
    RegisterView* m_reg;
    NoticeCard *m_noticeInline, *m_noticeExit;
    QLabel* m_footnote;
};

/// Experiment 04: Young's double slit.
class WavePage : public ExperimentPage {
    Q_OBJECT
public:
    explicit WavePage(AppContext* context, QWidget* parent = nullptr);
protected:
    void apply(bool record) override;
    void reset() override;
private:
    void onPresetChosen();
    void onWavelengthEdited();

    QComboBox* m_source;
    UnitField *m_lambda, *m_d, *m_L, *m_order;
    UnitField *m_sigmaLambda, *m_sigmaD, *m_sigmaL;
    UnitField *m_slitWidth, *m_intensityRatio;
    QCheckBox* m_chkExact;
    QCheckBox* m_chkEnvelope;
    ModeTabs* m_view;
    WaveCanvas* m_canvas;
    LineChart* m_chart;
    Panel *m_diagram, *m_chartPanel, *m_results;
    RegisterView *m_reg1, *m_reg2;
    NoticeCard* m_notice;
};

} // namespace phy104
