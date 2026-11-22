#include "ExperimentBase.h"

#include "AppContext.h"
#include "Components.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QStringList>
#include <QVBoxLayout>

namespace phy104 {

// ============================================================================ ExperimentBase (shared)
ExperimentBase::ExperimentBase(const QString& title, const QString& subtitle, const QString& experiment,
                               AppContext* ctx, QWidget* parent)
    : BasePage(title, subtitle, ctx, parent), m_experiment(experiment) {}

void ExperimentBase::bind(const QString& id, UnitField* field) { m_bindings.push_back({id, field, 0, false}); }

ExperimentBase::Binding* ExperimentBase::find(const QString& id) {
    for (Binding& b : m_bindings) if (b.id == id) return &b;
    return nullptr;
}

bool ExperimentBase::readFields() {
    bool allGood = true;
    for (Binding& b : m_bindings) {
        bool ok = false;
        b.field->value(&ok);
        b.field->setError(!ok, "Enter a number or expression.");
        allGood = allGood && ok;
    }
    if (!allGood) {                                                  // nothing can be calculated from unreadable text
        m_calc->setEnabled(false);
        emit m_ctx->message("Enter numbers only (for example 1.5 or 0.5*mm)");
    }
    return allGood;
}

void ExperimentBase::captureValid() {
    for (Binding& b : m_bindings) { bool ok = false; const double v = b.field->value(&ok); if (ok) { b.lastValid = v; b.hasValid = true; } }
}

QString ExperimentBase::lastRunLabel() const {
    for (auto it = m_ctx->log.runs().rbegin(); it != m_ctx->log.runs().rend(); ++it)
        if (it->experiment == m_experiment.toStdString()) return q(SessionLog::runLabel(it->id)).replace("#", "RUN ");
    return {};
}

ReportContext ExperimentBase::reportContext() const {
    return {m_haveLast ? &m_last : nullptr, lastRunLabel().toStdString()};
}

void ExperimentBase::setPanels(Panel* d, Panel* c, Panel* r, const QString& dm, const QString& cm) {
    m_diagramPanel = d; m_chartPanel = c; m_resultsPanel = r; m_diagramMeta = dm; m_chartMeta = cm;
}

void ExperimentBase::refreshMetas(bool paused) {
    const QString run = lastRunLabel();
    const QString number = run.section(' ', 1);
    if (m_diagramPanel) m_diagramPanel->setMeta(paused ? QString("LAST VALID") + (number.isEmpty() ? QString() : QString(" ") + number) : m_diagramMeta);
    if (m_chartPanel) m_chartPanel->setMeta(paused ? QString("STALE") + (run.isEmpty() ? QString() : QString(" ") + run) : m_chartMeta);
    if (m_resultsPanel) m_resultsPanel->setMeta(paused ? "PAUSED" : m_ctx->nextRunLabel());
}

void ExperimentBase::setNormalHint(const QString& text) {
    m_hintText = text;
    if (!m_restore->isVisible()) m_hint->setText(text);
}

QWidget* ExperimentBase::diagramCanvas() const {
    if (!m_diagramPanel) return nullptr;
    QVBoxLayout* body = m_diagramPanel->body();
    for (int i = 0; i < body->count(); ++i)
        if (QWidget* w = body->itemAt(i)->widget()) return w;      // the canvas inside the panel
    return nullptr;
}

QWidget* ExperimentBase::chartWidget() const {
    if (!m_chartPanel) return nullptr;
    QVBoxLayout* body = m_chartPanel->body();
    for (int i = 0; i < body->count(); ++i)
        if (QWidget* w = body->itemAt(i)->widget()) return w;      // the LineChart inside the panel
    return nullptr;
}

void ExperimentBase::buildButtons(QVBoxLayout* fields, const QString& hint) {
    m_calc = new QPushButton("Calculate & Simulate");
    m_calc->setObjectName("primary");
    m_calc->setProperty("primaryAction", true);
    m_calc->setProperty("controlId", QStringLiteral("calculate"));
    m_reset = new QPushButton("Reset parameters");
    m_reset->setObjectName("secondary");
    m_reset->setProperty("controlId", QStringLiteral("resetParameters"));
    m_restore = new QPushButton;
    m_restore->setObjectName("primary");
    m_restore->setProperty("controlId", QStringLiteral("restoreInput"));
    m_restore->hide();
    for (auto* b : {m_calc, m_reset, m_restore}) b->setCursor(Qt::PointingHandCursor);
    m_hint = new QLabel(hint);
    m_hint->setObjectName("hint");
    m_hint->setWordWrap(true);
    m_hintText = hint;
    fields->addSpacing(2);
    // One shared placement for every experiment: calculate and append the latest valid result.
    auto* send = new QPushButton("Send to Notes");
    send->setObjectName("sendToNotes");
    send->setAccessibleName("Send to Notes");
    send->setCursor(Qt::PointingHandCursor);
    send->setMinimumHeight(32);
    send->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    send->setToolTip("Append the latest valid result to session Notes.");
    m_calc->setText("Calculate & Simulate");
    m_calc->setMinimumHeight(34);
    m_calc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* row = new QHBoxLayout;
    row->setSpacing(6);
    row->addWidget(m_calc);
    row->addWidget(send);
    fields->addLayout(row);
    connect(send, &QPushButton::clicked, this, [this] { sendToNotes(); });
    fields->addWidget(m_reset);
    fields->addWidget(m_restore);
    fields->addWidget(m_hint);
    fields->addStretch();
    connect(m_calc, &QPushButton::clicked, this, [this] { apply(true); });
    connect(m_reset, &QPushButton::clicked, this, [this] { reset(); });
    connect(m_restore, &QPushButton::clicked, this, [this] {
        for (Binding& b : m_bindings)
            if (b.field->hasError() && b.hasValid) b.field->setValue(b.lastValid, b.field->decimals());
        apply(false);
    });
}

void ExperimentBase::applyIssues(const Report& r) {
    for (Binding& b : m_bindings) b.field->setError(false);
    const bool paused = r.paused;
    Binding* firstWithValue = nullptr;
    QString restoreLabel;
    for (const FieldIssue& issue : r.issues)
        if (Binding* b = find(q(issue.field))) {
            b->field->setError(true, q(issue.message));
            if (!firstWithValue && b->hasValid) { firstWithValue = b; restoreLabel = q(issue.label); }
        }
    m_calc->setEnabled(!paused);
    m_reset->setVisible(!paused);
    m_restore->setVisible(paused && firstWithValue);
    if (paused && firstWithValue) {
        QString unit = firstWithValue->field->unit();
        if (unit == "integer" || unit == "order" || unit == "slits") unit.clear();            // labels, not units
        const double v = firstWithValue->lastValid;
        m_restore->setText(QString(" ") + restoreLabel + QString(" ") + (firstWithValue->field->showsPlus() && v > 0 ? "+" : "") +
                           QString::number(v, 'f', firstWithValue->field->decimals()) + (unit.isEmpty() ? "" : QString(" ") + unit));
    }
    m_hint->setText(paused ? (firstWithValue ? "Restore the last-valid value, then calculate again." : "Edit the highlighted field to continue.") : m_hintText);

    StatusInfo info;
    if (paused) {
        const QString text = q(r.statusText);
        QString tail = text.section(" · ", 1);
        if (!tail.isEmpty()) tail[0] = tail[0].toUpper();
        info = {"Paused", text.section(" · ", 0, 0), tail, true};
    } else if (!r.statusText.empty()) {
        info.mid = q(r.statusText);
    }
    setStatusInfo(info, paused);
    refreshMetas(paused);
}

void ExperimentBase::sendToNotes() {
    if (!m_haveLast) { emit m_ctx->message("Nothing to send yet: calculate first"); return; }
    QString md = QString(" ") + m_experiment + QString(" ") + q(m_last.headline) + "\n";
    for (const auto& [name, value] : m_last.parameters) md += QString(" ") + q(name) + QString(" ") + q(value) + "\n";
    for (const LcdRegister& r : m_last.registers) {
        md += QString("*") + q(r.title) + "**: ";
        QStringList parts;
        for (const auto& l : r.lines) parts << q(l).trimmed();
        md += parts.join("; ");
        if (!r.resultValue.empty()) md += QString(" ") + q(r.resultLabel) + QString(" ") + q(r.resultValue);
        md += "\n";
    }
    m_ctx->appendNotes(md);
}

void ExperimentBase::showRegisters(RegisterView* a, RegisterView* b, const Report& r) {
    if (!r.registers.empty()) a->setRegister(r.registers[0]);
    if (r.registers.size() > 1) { b->setRegister(r.registers[1]); b->show(); } else b->hide();
}

} // namespace phy104
