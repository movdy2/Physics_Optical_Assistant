#include "ExperimentBase.h"

#include "AppContext.h"
#include "Components.h"
#include "LanguageManager.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSizePolicy>
#include <QStringList>
#include <QVBoxLayout>

namespace phy104 {

QString q(const std::string& s) { return QString::fromStdString(s); }

// ============================================================================ ExperimentBase (shared)
ExperimentBase::ExperimentBase(const QString& title, const QString& subtitle, const QString& experiment,
                               AppContext* ctx, QWidget* parent)
    : BasePage(title, subtitle, ctx, parent), m_experiment(experiment) {}

void ExperimentBase::bind(const QString& id, UnitField* field) { m_bindings.push_back({id, field, 0, false}); }

ExperimentBase::Binding* ExperimentBase::find(const QString& id) {
    for (Binding& b : m_bindings) if (b.id == id) return &b;
    return nullptr;
}

void ExperimentBase::saveState(QSettings& settings) const {
    const QString group = QStringLiteral("ui/experimentState/%1").arg(m_experiment);
    settings.beginGroup(group);
    for (const Binding& b : m_bindings) {
        bool ok = false;
        const double v = b.field->value(&ok);
        if (ok) settings.setValue(b.id, v);
    }
    settings.endGroup();
}

void ExperimentBase::restoreState(QSettings& settings) {
    if (m_stateRestored) return;
    const QString group = QStringLiteral("ui/experimentState/%1").arg(m_experiment);
    settings.beginGroup(group);
    for (Binding& b : m_bindings) {
        const QVariant stored = settings.value(b.id);
        if (!stored.isValid()) continue;
        b.field->setValue(stored.toDouble(), b.field->decimals());
    }
    settings.endGroup();
    m_stateRestored = true;
}

bool ExperimentBase::readFields() {
    bool allGood = true;
    for (Binding& b : m_bindings) {
        bool ok = false;
        b.field->value(&ok);
        b.field->setError(!ok, LanguageManager::instance().text("Enter a number or expression."));
        allGood = allGood && ok;
    }
    if (!allGood) {                                                  // no calculation is attempted from unreadable input
        m_calc->setEnabled(false);
        Binding* firstInvalid = nullptr;
        for (Binding& b : m_bindings) {
            bool ok = false;
            b.field->value(&ok);
            if (!ok) { firstInvalid = &b; break; }
        }
        const bool canRestore = firstInvalid && firstInvalid->hasValid;
        m_reset->setVisible(false);
        m_restore->setVisible(canRestore);
        if (canRestore) m_restore->setText("Restore last valid value");
        const QString noticeTitle = LanguageManager::instance().text("Invalid input");
        const QString noticeBody = LanguageManager::instance().text(
            "Enter a number or expression, such as 1.5 or 0.5*mm. The last valid diagram and result remain visible until corrected.");
        for (NoticeCard* notice : findChildren<NoticeCard*>()) {
            notice->setNotice(noticeTitle, noticeBody, true);
            notice->show();
            break;
        }
        if (m_hint) m_hint->setText(LanguageManager::instance().text(m_haveLast
            ? "Correct the highlighted input; the last valid result is retained."
            : "Correct the highlighted input, then calculate again."));
        StatusInfo info{"Paused", LanguageManager::instance().text("Invalid input"),
                        LanguageManager::instance().text("Correct the highlighted field"), true};
        setStatusInfo(info, true);
        refreshMetas(true);
        emit m_ctx->message(LanguageManager::instance().text("Enter a number or expression, such as 1.5 or 0.5*mm."));
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
    const QString isolatedNumber = number.isEmpty() ? QString() : QString(QChar(0x2066)) + number + QChar(0x2069);
    const QString isolatedRun = run.isEmpty() ? QString() : QString(QChar(0x2066)) + run + QChar(0x2069);
    if (m_diagramPanel) m_diagramPanel->setMeta(paused
        ? QString("LAST VALID") + (isolatedNumber.isEmpty() ? QString() : QString(" ") + isolatedNumber)
        : m_diagramMeta);
    if (m_chartPanel) m_chartPanel->setMeta(paused
        ? QString("STALE") + (isolatedRun.isEmpty() ? QString() : QString(" · ") + isolatedRun)
        : m_chartMeta);
    if (m_resultsPanel) {
        if (!paused) m_resultsPanel->setMeta(m_ctx->nextRunLabel());
        else if (m_haveLast) m_resultsPanel->setMeta("STALE",
            "Last valid result retained while the highlighted input is corrected.");
        else m_resultsPanel->setMeta("INPUT ERROR",
            "Correct the highlighted input before the first calculation.");
    }
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
    auto* send = new QPushButton("Send to\nNotes");
    send->setObjectName("sendToNotes");
    send->setAccessibleName("Send to Notes");
    send->setCursor(Qt::PointingHandCursor);
    send->setMinimumHeight(32);
    send->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    send->setToolTip("Append the latest valid result to session Notes.");
    m_calc->setText("Calculate & Simulate");
    m_calc->setMinimumHeight(34);
    m_calc->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto* actionRow = new QHBoxLayout;
    actionRow->setSpacing(6);
    actionRow->addWidget(m_calc, 1, Qt::AlignTop);
    actionRow->addWidget(send, 1, Qt::AlignTop);
    fields->addLayout(actionRow);
    connect(send, &QPushButton::clicked, this, [this] { sendToNotes(); });
    fields->addWidget(m_reset);
    fields->addWidget(m_restore);
    fields->addWidget(m_hint);
    fields->addStretch();
    connect(m_calc, &QPushButton::clicked, this, [this] { apply(true); });
    connect(m_reset, &QPushButton::clicked, this, [this] {
        reset();                                 // defaults for this module
        QSettings settings(QStringLiteral("PHY104"), QStringLiteral("PHY104OpticalAssistant"));
        saveState(settings);                     // remember the reset state
    });
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
        const QString value = QString(QChar(0x2066)) +
            (firstWithValue->field->showsPlus() && v > 0 ? QString("+") : QString()) +
            QString::number(v, 'f', firstWithValue->field->decimals()) +
            (unit.isEmpty() ? QString() : QString(" ") + unit) + QChar(0x2069);
        m_restore->setText(restoreLabel + QString(" ") + value);
    }
    m_hint->setText(LanguageManager::instance().text(paused
        ? (firstWithValue ? "Restore the last-valid value, then calculate again." : "Edit the highlighted field to continue.")
        : m_hintText));

    StatusInfo info;
    if (paused) {
        const QString text = q(r.statusText);
        QString tail = text.section(" · ", 1);
        if (!tail.isEmpty()) tail[0] = tail[0].toUpper();
        info = {LanguageManager::instance().text("Paused"),
                LanguageManager::instance().text(text.section(" · ", 0, 0)),
                LanguageManager::instance().text(tail), true};
    } else if (!r.statusText.empty()) {
        info.mid = q(r.statusText);
    }
    setStatusInfo(info, paused);
    refreshMetas(paused);
}

void ExperimentBase::sendToNotes() {
    if (!m_haveLast) { emit m_ctx->message("Nothing to send yet: calculate first"); return; }
    QString md = QString("### ") + m_experiment + "\n\n" + q(m_last.headline) + "\n";
    for (const auto& [name, value] : m_last.parameters)
        md += QString("- ") + q(name) + " = " + q(value) + "\n";
    for (const LcdRegister& r : m_last.registers) {
        md += QString("- **") + q(r.title) + "**: ";
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
