#include "MathSolverPage.h"

#include "AppContext.h"
#include "Components.h"
#include "ThemeManager.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QVBoxLayout>

namespace phy104 {

MathSolverPage::MathSolverPage(AppContext* context, QWidget* parent)
    : BasePage("Calculator", "", context, parent), m_calc(5) {
    setObjectName("calcPage");

    auto* columns = new QHBoxLayout;
    columns->setSpacing(12);

    auto* calculatorPanel = new Panel("Expression");
    auto* modeRow = new QHBoxLayout;
    m_angleButton = new QPushButton("DEG");
    m_angleButton->setObjectName("calcMode");
    m_angleButton->setCheckable(true);
    m_angleButton->setChecked(true);                         // the school default; checked = degrees
    m_angleButton->setToolTip("Switch between degrees and radians");
    modeRow->addStretch();
    modeRow->addWidget(m_angleButton);
    calculatorPanel->body()->addLayout(modeRow);

    m_input = new QLineEdit;
    m_input->setObjectName("lcdInput");
    m_input->setLayoutDirection(Qt::LeftToRight);
    m_input->setPlaceholderText("Expression");
    const auto refreshPlaceholder = [this] {
        QPalette palette = m_input->palette();
        const Theme& theme = ThemeManager::instance().theme();
        palette.setColor(QPalette::PlaceholderText, theme.dark ? QColor(Qt::white) : theme.text);
        m_input->setPalette(palette);
    };
    refreshPlaceholder();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshPlaceholder);
    calculatorPanel->body()->addWidget(m_input);

    m_result = new QLabel("0");
    m_result->setObjectName("lcdResult");
    m_result->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_result->setMinimumHeight(64);
    calculatorPanel->body()->addWidget(m_result);

    m_status = new QLabel;
    m_status->setObjectName("statusError");
    m_status->setWordWrap(true);
    m_status->hide();
    calculatorPanel->body()->addWidget(m_status);

    m_keypad = new CalculatorKeypad;
    m_keypad->setObjectName("calculatorKeypad");
    calculatorPanel->body()->addWidget(m_keypad, 1);
    columns->addWidget(calculatorPanel, 3);
    content()->addLayout(columns, 1);

    connect(m_input, &QLineEdit::returnPressed, this, &MathSolverPage::calculate);
    connect(m_keypad, &CalculatorKeypad::equals, this, &MathSolverPage::calculate);
    connect(m_keypad, &CalculatorKeypad::clearAll, this, &MathSolverPage::clearAll);
    connect(m_keypad, &CalculatorKeypad::backspace, this, [this] { m_input->backspace(); m_input->setFocus(); });
    connect(m_keypad, &CalculatorKeypad::insertText, this, [this](const QString& text) {
        m_input->insert(text);
        m_input->setFocus();
    });
    connect(m_angleButton, &QPushButton::toggled, this, &MathSolverPage::toggleAngleMode);
}

void MathSolverPage::calculate() {
    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;
    const CalcEntry entry = m_calc.evaluate(text.toStdString());

    if (entry.ok) {
        m_result->setText(QString::fromStdString(entry.result));
        m_status->hide();
        m_input->selectAll();                                      // next typing replaces the previous expression
    } else {
        m_result->setText("ERROR");
        m_status->setText("✖ " + QString::fromStdString(entry.error));
        m_status->show();
    }
}

void MathSolverPage::clearAll() {
    m_calc.clearHistory();
    m_input->clear();
    m_result->setText("0");
    m_status->hide();
    m_input->setFocus();
}

void MathSolverPage::toggleAngleMode(bool degrees) {
    m_calc.setAngleUnit(degrees ? AngleUnit::Degrees : AngleUnit::Radians);
    m_angleButton->setText(degrees ? "DEG" : "RAD");
}

} // namespace phy104
