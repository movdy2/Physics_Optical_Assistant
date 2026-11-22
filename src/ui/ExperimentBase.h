#pragma once
#include "BasePage.h"
#include "ExperimentReports.h"

#include <QSettings>
#include <vector>

class QLabel;
class QPushButton;
class QVBoxLayout;

class Panel;
class RegisterView;
class UnitField;

namespace phy104 {

class AppContext;

/// Abstract base shared by all 11 experiment modules.
///
/// ExperimentBase is a QWidget subclass (via BasePage) and owns everything that is
/// identical across the experiment screens:
///   * the page title / "Calculate & Simulate" / "Save & Export" header (BasePage)
///   * the Calculate & Simulate, Send to Notes, Reset and Restore buttons
///   * bound input fields with the paused / last-valid pattern
///   * the right-side Results Panel and its two LCD registers
///   * shared logic: send-to-notes, register (LCD) text updates, apply-issues
///   * per-module field persistence via QSettings (saveState / restoreState)
///
/// Concrete modules (Refraction, Lenses, Prisms, Wave, Newton's Rings, Michelson,
/// Thin-Film, Grating, Single-Slit, Polarization, Thick Lens) inherit from it —
/// directly or via ExperimentPage / ModelPage — and implement apply() / reset().
class ExperimentBase : public BasePage {
    Q_OBJECT
public:
    ExperimentBase(const QString& title, const QString& subtitle, const QString& experiment,
                   AppContext* context, QWidget* parent = nullptr);

    /// Persist every bound input field's current value under
    /// `ui/experimentState/<experiment>/<fieldId>`.
    void saveState(QSettings& settings) const;
    /// Restore every bound input field from QSettings. Only takes effect the first
    /// time it is called for a given page instance (in-session edits are never
    /// clobbered by a later restore).
    void restoreState(QSettings& settings);
    /// True once restoreState() has been applied for this instance.
    bool stateRestored() const { return m_stateRestored; }

    /// The experiment name the session uses ("Thin-Film Interference", ...).
    QString experimentName() const { return m_experiment; }
    /// The live diagram canvas and chart, captured into the printable report.
    QWidget* diagramCanvas() const;
    QWidget* chartWidget() const;

protected:
    struct Binding { QString id; UnitField* field; double lastValid = 0; bool hasValid = false; };
    void bind(const QString& id, UnitField* field);
    bool readFields();                          // parses all bound fields; marks bad ones; false on failure
    void captureValid();                        // remembers the current field values as "last valid"
    void applyIssues(const Report& report);     // inline errors, buttons, status bar, engine light
    void showRegisters(RegisterView* first, RegisterView* second, const Report& report);
    ReportContext reportContext() const;
    QString lastRunLabel() const;               // "RUN 021" for this experiment, or ""
    void setPanels(Panel* diagram, Panel* chart, Panel* results,
                   const QString& diagramMeta, const QString& chartMeta);
    void refreshMetas(bool paused);
    void buildButtons(QVBoxLayout* fields, const QString& hint);  // Calc & Simulate / Send to Notes / Reset / Restore
    void sendToNotes();                         // appends the last valid result to session notes
    void setNormalHint(const QString& text);

    QString m_experiment;
    std::vector<Binding> m_bindings;
    Report m_last;
    bool m_haveLast = false;
    bool m_stateRestored = false;   // restoreState() applied once per instance
    QPushButton *m_calc = nullptr, *m_reset = nullptr, *m_restore = nullptr;
    QLabel* m_hint = nullptr;

    virtual void apply(bool record) = 0;
    virtual void reset() = 0;

private:
    Binding* find(const QString& id);
    Panel *m_diagramPanel = nullptr, *m_chartPanel = nullptr, *m_resultsPanel = nullptr;
    QString m_diagramMeta, m_chartMeta, m_hintText;
};

} // namespace phy104
