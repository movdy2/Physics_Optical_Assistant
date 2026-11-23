#pragma once
#include "ExperimentPages.h"
#include "WaveExperiments.h"

class QSlider;
class QTimer;

namespace phy104 {
class ResultCard;

/// Shared body of the six screens of the second Figma batch (header with Save / Export, tabs + fields on the
/// left, diagram + chart in the middle, two LCD registers + Interpretation / Input guard on the right).
/// A subclass builds its fields in the constructor and implements compute() / updateCanvas() / resetFields().
class ModelPage : public ExperimentPage {
    Q_OBJECT
protected:
    ModelPage(const QString& title, const QString& subtitle, const QString& experiment, AppContext* context, QWidget* parent);

    UnitField* addField(const QString& id, const QString& label, const QString& unit, double value, int decimals, bool signedValue = false, bool spinner = false, double step = 1);
    /// Dropdown whose items carry a number (item data).
    QComboBox* addCombo(const QString& label, const QStringList& items, const QList<double>& values);
    ModeTabs* addTabs(const QStringList& tabs);
    /// Builds the three columns; call once, after the fields were added.
    void build(const QString& diagramTitle, QWidget* canvas, const QString& chartTitle, const QString& hint);
    /// Re-captions the diagram and chart panels (the second tabs show a different picture).
    void retitle(const QString& diagramTitle, const QString& chartTitle);
    /// Use native-theme result cards instead of legacy calculation-register styling.
    void useNativeResultCards();

    virtual ModelReport compute(const ReportContext& ctx) = 0;
    virtual void updateCanvas() = 0;
    virtual void resetFields() = 0;
    virtual QString runDescription() const = 0;

    /// Calculate and repaint, with optional slider debounce (default: immediate).
    void apply(bool record) override;
    void reset() override;
    /// Debounced apply: coalesces rapid slider valueChanged calls into a single apply after 16 ms.
    Q_SLOT void debounceApply();

    QVBoxLayout* m_fieldLayout = nullptr;
    ModeTabs* m_tabs = nullptr;

protected:
    Panel* m_diagram;                          // diagram panel (subclasses may add help button)
private:
    void showChart(const ChartData& c);
    Panel *m_controls, *m_chartPanel, *m_results;
    LineChart* m_chart;
    RegisterView *m_reg1, *m_reg2;
    ResultCard *m_resultCard1, *m_resultCard2;
    NoticeCard *m_interpretation, *m_guard;
    bool m_useNativeResultCards = false;
    QString m_diagramTitle;
    QList<QWidget*> m_pending;                 // controls added before build()
    QTimer* m_debounceTimer = nullptr;         // 16 ms slider debounce
};

class NewtonRingsPage : public ModelPage {
    Q_OBJECT
public:
    explicit NewtonRingsPage(AppContext* ctx, QWidget* parent = nullptr);
protected:
    ModelReport compute(const ReportContext& ctx) override;
    void updateCanvas() override;
    void resetFields() override;
    QString runDescription() const override;
private:
    UnitField *m_lambda, *m_radius, *m_order;
    UnitField *m_sigmaLambda, *m_sigmaRadius, *m_sigmaN;
    QComboBox *m_medium, *m_viewing;
    QPushButton *m_presetAir, *m_presetWater, *m_presetOil, *m_presetTransmitted;
    bool m_syncing = false;
    class NewtonCanvas* m_canvas;
};

class MichelsonPage : public ModelPage {
    Q_OBJECT
public:
    explicit MichelsonPage(AppContext* ctx, QWidget* parent = nullptr);
protected:
    ModelReport compute(const ReportContext& ctx) override;
    void updateCanvas() override;
    void resetFields() override;
    QString runDescription() const override;
private:
    void setMirrorDisplacement(double displacementUm);
    MichelsonInput currentInput() const;

    UnitField *m_lambda, *m_dx, *m_coherenceLength, *m_plateThickness, *m_plateIndex;
    UnitField *m_fringeCount, *m_viewAngle;
    QComboBox* m_solveFor;
    QSlider* m_dxSlider;
    QSlider* m_nSlider;
    QSlider* m_tiltSlider;
    QComboBox *m_source, *m_medium, *m_reference, *m_balance;
    QPushButton *m_presetAir, *m_presetCrown, *m_presetFlint, *m_presetFringes;
    bool m_syncing = false;
    class MichelsonCanvas* m_canvas;
};

class ThinFilmPage : public ModelPage {
    Q_OBJECT
public:
    explicit ThinFilmPage(AppContext* ctx, QWidget* parent = nullptr);
    /// The swatch of the "Colours" tab ("#rrggbb"), empty on the other tabs.
    QString colourSwatch();
protected:
    ModelReport compute(const ReportContext& ctx) override;
    void updateCanvas() override;
    void resetFields() override;
    QString runDescription() const override;
private:
    UnitField *m_lambda, *m_n, *m_n2, *m_t, *m_theta;
    QPushButton *m_presetSoap, *m_presetOil, *m_presetTransmission;
    bool m_syncing = false;
    QLabel* m_swatch = nullptr;
    class ThinFilmCanvas* m_canvas;
};

class GratingPage : public ModelPage {
    Q_OBJECT
public:
    explicit GratingPage(AppContext* ctx, QWidget* parent = nullptr);
protected:
    ModelReport compute(const ReportContext& ctx) override;
    void updateCanvas() override;
    void resetFields() override;
    QString runDescription() const override;
private:
    UnitField *m_density, *m_lambda, *m_lambda2, *m_order, *m_slits;
    QComboBox* m_incidence;
    QPushButton *m_presetStandard, *m_presetHighDensity, *m_presetResolution;
    bool m_syncing = false;
    class GratingCanvas* m_canvas;
};

class SingleSlitPage : public ModelPage {
    Q_OBJECT
public:
    explicit SingleSlitPage(AppContext* ctx, QWidget* parent = nullptr);
protected:
    ModelReport compute(const ReportContext& ctx) override;
    void updateCanvas() override;
    void resetFields() override;
    QString runDescription() const override;
private:
    UnitField *m_width, *m_lambda, *m_screen, *m_separation;
    QComboBox *m_wave, *m_scale, *m_exactGeom;
    QCheckBox *m_showEnvelopeOnly;
    QPushButton *m_presetStandard, *m_presetGeometry, *m_presetEnvelope;
    bool m_syncing = false;
    class SingleSlitCanvas* m_canvas;
};

class PolarizationPage : public ModelPage {
    Q_OBJECT
public:
    explicit PolarizationPage(AppContext* ctx, QWidget* parent = nullptr);
protected:
    ModelReport compute(const ReportContext& ctx) override;
    void updateCanvas() override;
    void resetFields() override;
    QString runDescription() const override;
private:
    UnitField *m_intensity, *m_angle, *m_n1, *m_n2;
    QComboBox* m_beam;
    QPushButton *m_presetMalus, *m_presetBrewster;
    bool m_syncing = false;
    class PolarizationCanvas* m_canvas;
};

} // namespace phy104
