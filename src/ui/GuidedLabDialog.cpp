#include "GuidedLabDialog.h"

#include "AppContext.h"
#include "LanguageManager.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace phy104 {
namespace {

QString ltrIsolate(const QString& value) {
    return QString(QChar(0x2066)) + value + QChar(0x2069);
}

QWidget* stepPage(const QString& headingText, const QString& detailText, QStackedWidget* stack) {
    auto* page = new QWidget;
    page->setObjectName("guidedLabStep");
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);
    auto* heading = new QLabel(headingText);
    heading->setObjectName("guidedLabHeading");
    heading->setWordWrap(true);
    auto* detail = new QLabel(detailText);
    detail->setObjectName("guidedLabDetail");
    detail->setWordWrap(true);
    layout->addWidget(heading);
    layout->addWidget(detail);
    layout->addStretch();
    stack->addWidget(page);
    return page;
}

} // namespace

GuidedLabDialog::GuidedLabDialog(AppContext* context, std::function<void()> applyStarterValues,
                                 QWidget* parent)
    : QDialog(parent), m_context(context), m_applyStarterValues(std::move(applyStarterValues)) {
    setObjectName("guidedLabDialog");
    setWindowTitle("Guided lab · Refraction & TIR");
    setModal(false);
    setMinimumSize(540, 420);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 18, 22, 18);
    root->setSpacing(12);

    auto* intro = new QLabel("A short investigation using Snell's law. Keep this guide open while you work on the experiment.");
    intro->setObjectName("guidedLabIntro");
    intro->setWordWrap(true);
    root->addWidget(intro);

    m_stepLabel = new QLabel;
    m_stepLabel->setObjectName("guidedLabProgress");
    root->addWidget(m_stepLabel);

    m_steps = new QStackedWidget;
    m_steps->setObjectName("guidedLabSteps");
    QWidget* predictionPage = stepPage("1 · Make a prediction",
        "Question: when light travels from air into glass, does the ray bend toward or away from the normal? Write what you expect before running the model.", m_steps);
    m_prediction = new QPlainTextEdit;
    m_prediction->setObjectName("guidedLabPrediction");
    m_prediction->setPlaceholderText("My prediction and why…");
    m_prediction->document()->setMaximumBlockCount(20);
    qobject_cast<QVBoxLayout*>(predictionPage->layout())->insertWidget(2, m_prediction, 1);

    QWidget* setupPage = stepPage("2 · Set up the trial",
        "Use air as the incident medium and glass as the transmitted medium. Start at 30° and keep the uncertainty fields at zero for this first pass.", m_steps);
    auto* formula = new QLabel("n₁ sin θ₁ = n₂ sin θ₂");
    formula->setObjectName("guidedLabFormula");
    formula->setLayoutDirection(Qt::LeftToRight);
    formula->setAlignment(Qt::AlignCenter);
    qobject_cast<QVBoxLayout*>(setupPage->layout())->insertWidget(2, formula);
    auto* applyPreset = new QPushButton("Use suggested values · air → glass · 30°");
    applyPreset->setObjectName("secondary");
    applyPreset->setCursor(Qt::PointingHandCursor);
    qobject_cast<QVBoxLayout*>(setupPage->layout())->insertWidget(3, applyPreset);
    connect(applyPreset, &QPushButton::clicked, this, [this] {
        if (m_applyStarterValues) m_applyStarterValues();
    });

    QWidget* runPage = stepPage("3 · Run and observe",
        "Return to the experiment and press Calculate & Simulate. Compare the ray diagram and calculated angle with your prediction, then continue here.", m_steps);
    auto* predictionCaption = new QLabel("Your prediction");
    predictionCaption->setObjectName("fieldLabel");
    m_predictionEcho = new QLabel("No prediction entered yet.");
    m_predictionEcho->setObjectName("guidedLabPredictionEcho");
    m_predictionEcho->setWordWrap(true);
    auto* resultCaption = new QLabel("Latest model result");
    resultCaption->setObjectName("fieldLabel");
    m_result = new QLabel("No result published yet.");
    m_result->setObjectName("guidedLabResult");
    m_result->setWordWrap(true);
    m_result->setLayoutDirection(Qt::LeftToRight);
    qobject_cast<QVBoxLayout*>(runPage->layout())->insertWidget(2, predictionCaption);
    qobject_cast<QVBoxLayout*>(runPage->layout())->insertWidget(3, m_predictionEcho);
    qobject_cast<QVBoxLayout*>(runPage->layout())->insertWidget(4, resultCaption);
    qobject_cast<QVBoxLayout*>(runPage->layout())->insertWidget(5, m_result);
    connect(m_prediction, &QPlainTextEdit::textChanged, this, [this] {
        const QString prediction = m_prediction->toPlainText().trimmed();
        m_predictionEcho->setText(prediction.isEmpty()
            ? LanguageManager::instance().text("No prediction entered yet.") : prediction);
    });

    QWidget* reflectionPage = stepPage("4 · Reflect and save",
        "What happened to the ray? How did the result compare with your prediction? Record a short reflection in the lab notes.", m_steps);
    m_reflection = new QPlainTextEdit;
    m_reflection->setObjectName("guidedLabReflection");
    m_reflection->setPlaceholderText("Observation and reflection…");
    m_reflection->document()->setMaximumBlockCount(20);
    qobject_cast<QVBoxLayout*>(reflectionPage->layout())->insertWidget(2, m_reflection, 1);
    m_addToNotes = new QPushButton("Add lab report to Notes");
    m_addToNotes->setObjectName("primary");
    m_addToNotes->setCursor(Qt::PointingHandCursor);
    qobject_cast<QVBoxLayout*>(reflectionPage->layout())->insertWidget(3, m_addToNotes);
    m_savedStatus = new QLabel;
    m_savedStatus->setObjectName("guidedLabSavedStatus");
    m_savedStatus->setWordWrap(true);
    qobject_cast<QVBoxLayout*>(reflectionPage->layout())->insertWidget(4, m_savedStatus);
    connect(m_addToNotes, &QPushButton::clicked, this, &GuidedLabDialog::addReportToNotes);
    m_addToNotes->setEnabled(false);
    const auto updateReportAvailability = [this] {
        m_addToNotes->setEnabled(!m_prediction->toPlainText().trimmed().isEmpty() &&
                                 !m_reflection->toPlainText().trimmed().isEmpty());
    };
    connect(m_prediction, &QPlainTextEdit::textChanged, this, updateReportAvailability);
    connect(m_reflection, &QPlainTextEdit::textChanged, this, updateReportAvailability);

    root->addWidget(m_steps, 1);
    auto* navigation = new QHBoxLayout;
    m_back = new QPushButton("← Back");
    m_back->setObjectName("secondary");
    m_next = new QPushButton("Next →");
    m_next->setObjectName("primary");
    auto* closeButton = new QPushButton("Close");
    closeButton->setObjectName("flat");
    navigation->addWidget(m_back);
    navigation->addStretch();
    navigation->addWidget(closeButton);
    navigation->addWidget(m_next);
    root->addLayout(navigation);

    connect(m_back, &QPushButton::clicked, this, [this] { setStep(m_currentStep - 1); });
    connect(m_next, &QPushButton::clicked, this, [this] {
        if (m_currentStep == m_steps->count() - 1) close();
        else setStep(m_currentStep + 1);
    });
    connect(closeButton, &QPushButton::clicked, this, &QDialog::close);
    connect(m_context, &AppContext::snapshotChanged, this, &GuidedLabDialog::refreshResult);

    LanguageManager::instance().apply(this);
    setStep(0);
    refreshResult();
}

void GuidedLabDialog::setStep(int step) {
    m_currentStep = std::clamp(step, 0, m_steps->count() - 1);
    m_steps->setCurrentIndex(m_currentStep);
    const QStringList englishNames{"Prediction", "Set up", "Observe", "Reflect"};
    const QStringList arabicNames{"التوقع", "الإعداد", "الملاحظة", "التأمل"};
    const QString name = LanguageManager::instance().isArabic() ? arabicNames[m_currentStep] : englishNames[m_currentStep];
    m_stepLabel->setText(LanguageManager::instance().isArabic()
        ? QString("الخطوة %1 من ٤ · %2").arg(ltrIsolate(QString::number(m_currentStep + 1)), name)
        : QString("Step %1 of 4 · %2").arg(m_currentStep + 1).arg(name));
    m_back->setEnabled(m_currentStep > 0);
    m_next->setText(m_currentStep == m_steps->count() - 1
        ? LanguageManager::instance().text("Finish")
        : LanguageManager::instance().text("Next →"));
}

void GuidedLabDialog::refreshResult() {
    const Snapshot* snapshot = m_context->log.snapshot("Refraction & TIR");
    if (!snapshot || snapshot->headline.empty()) {
        m_result->setText(LanguageManager::instance().text("No result published yet."));
        return;
    }
    m_result->setText(ltrIsolate(QString::fromStdString(snapshot->headline)));
}

void GuidedLabDialog::addReportToNotes() {
    const Snapshot* snapshot = m_context->log.snapshot("Refraction & TIR");
    const QString result = snapshot ? ltrIsolate(QString::fromStdString(snapshot->headline))
                                    : LanguageManager::instance().text("Not calculated yet");
    const auto text = [](const QString& source) { return LanguageManager::instance().text(source); };
    QString report = QStringLiteral("## ") + text("Guided lab · Refraction & TIR") + QStringLiteral("\n\n")
        + QStringLiteral("**") + text("Prediction") + QStringLiteral("**\n") + m_prediction->toPlainText().trimmed() + QStringLiteral("\n\n")
        + QStringLiteral("**") + text("Model result") + QStringLiteral("**\n") + result + QStringLiteral("\n\n")
        + QStringLiteral("**") + text("Observation and reflection") + QStringLiteral("**\n")
        + m_reflection->toPlainText().trimmed() + QStringLiteral("\n");
    m_context->appendNotes(report);
    m_savedStatus->setText(LanguageManager::instance().text("Lab report added to session Notes."));
    m_addToNotes->setEnabled(false);
}

} // namespace phy104
