#include "MathSolverPage.h"

#include "AppContext.h"
#include "Components.h"
#include "ThemeManager.h"

#include <QAbstractItemView>
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPalette>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace phy104 {

MathSolverPage::MathSolverPage(AppContext* context, QWidget* parent)
    : BasePage("Calculator", "", context, parent), m_calc(5) {
    setObjectName("calcPage");

    auto* columns = new QHBoxLayout;
    columns->setContentsMargins(0, 0, 0, 0);
    columns->setSpacing(12);
    columns->addStretch(1);

    auto* calculatorPanel = new Panel("Expression");
    calculatorPanel->setMinimumWidth(440);
    calculatorPanel->setMaximumWidth(820);
    calculatorPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

    auto* resultRow = new QHBoxLayout;
    resultRow->setContentsMargins(0, 0, 0, 0);
    resultRow->setSpacing(4);
    resultRow->addWidget(m_result, 1);
    calculatorPanel->body()->addLayout(resultRow);

    m_status = new QLabel;
    m_status->setObjectName("statusError");
    m_status->setWordWrap(true);
    m_status->hide();
    calculatorPanel->body()->addWidget(m_status);

    m_keypad = new CalculatorKeypad;
    m_keypad->setObjectName("calculatorKeypad");
    m_keypad->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    calculatorPanel->body()->addWidget(m_keypad);
    columns->addWidget(calculatorPanel, 3);

    auto* historyPanel = new Panel("Recent calculations", "ANS");
    historyPanel->setMinimumWidth(230);
    historyPanel->setMaximumWidth(300);
    historyPanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    auto* historyHint = new QLabel("Click an entry to reuse its expression. Ans keeps the last successful result.");
    historyHint->setObjectName("calcHint");
    historyHint->setWordWrap(true);
    historyPanel->body()->addWidget(historyHint);

    m_historyEmpty = new QLabel("No calculations yet");
    m_historyEmpty->setObjectName("calcHistoryEmpty");
    m_historyEmpty->setAlignment(Qt::AlignCenter);
    m_historyEmpty->setWordWrap(true);
    historyPanel->body()->addWidget(m_historyEmpty, 1);

    m_historyList = new QListWidget;
    m_historyList->setObjectName("calcHistory");
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_historyList->setWordWrap(true);
    historyPanel->body()->addWidget(m_historyList, 1);

    m_historyClear = new QPushButton("Clear history");
    m_historyClear->setObjectName("calcHistoryClear");
    m_historyClear->setToolTip("Clear recent calculations; keep Ans");
    m_historyClear->setEnabled(false);
    historyPanel->body()->addWidget(m_historyClear);

    columns->addWidget(historyPanel, 1);
    columns->addStretch(1);
    content()->addLayout(columns, 1);

    connect(m_input, &QLineEdit::returnPressed, this, &MathSolverPage::calculate);
    connect(m_keypad, &CalculatorKeypad::equals, this, &MathSolverPage::calculate);
    connect(m_keypad, &CalculatorKeypad::clearAll, this, &MathSolverPage::clearAll);
    connect(m_keypad, &CalculatorKeypad::backspace, this, [this] { m_input->backspace(); m_input->setFocus(); });
    connect(m_keypad, &CalculatorKeypad::insertText, this, [this](const QString& text) {
        const QString selected = m_input->selectedText();
        const bool unit = text == "nm" || text == "um" || text == "mm" || text == "cm";
        if (!selected.isEmpty() && (text == "^2" || text == "^3")) {
            m_input->insert(QString("(%1)%2").arg(selected, text));
        } else if (!selected.isEmpty() && text == "1/(") {
            m_input->insert(QString("1/(%1)").arg(selected));
        } else if (text == "1/(") {
            const QString expression = m_input->text();
            if (!expression.trimmed().isEmpty() && m_input->cursorPosition() == expression.size())
                m_input->setText(QString("1/(%1)").arg(expression));
            else
                m_input->insert(text);
        } else if (unit && !selected.isEmpty()) {
            m_input->insert(selected + QString("*") + text);
        } else if (unit) {
            const QString expression = m_input->text();
            int cursor = m_input->cursorPosition();
            while (cursor > 0 && expression.at(cursor - 1).isSpace()) --cursor;
            const bool followsOperand = cursor > 0 &&
                (expression.at(cursor - 1).isLetterOrNumber() || expression.at(cursor - 1) == ')' ||
                 expression.at(cursor - 1) == '.');
            m_input->insert(followsOperand ? QString("*") + text : text);
        } else {
            m_input->insert(text);
        }
        m_input->setFocus();
    });
    connect(m_angleButton, &QPushButton::toggled, this, &MathSolverPage::toggleAngleMode);
    connect(m_historyList, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        m_input->setText(item->data(Qt::UserRole).toString());
        m_input->setFocus();
        m_input->selectAll();
        m_status->hide();
    });
    connect(m_historyClear, &QPushButton::clicked, this, [this] {
        m_calc.clearHistoryEntries();
        refreshHistory();
        m_input->setFocus();
    });
    refreshHistory();
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
        m_status->setText(QString(" ") + QString::fromStdString(entry.error));
        m_status->show();
    }
    refreshHistory();
}

void MathSolverPage::clearAll() {
    m_calc.clearHistory();
    refreshHistory();
    m_input->clear();
    m_result->setText("0");
    m_status->hide();
    m_input->setFocus();
}

void MathSolverPage::refreshHistory() {
    if (!m_historyList) return;
    m_historyList->clear();
    const auto& entries = m_calc.history();
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        const QString expression = QString::fromStdString(it->expression);
        const QString result = it->ok ? QString("= %1").arg(QString::fromStdString(it->result))
                                      : QString("Error: %1").arg(QString::fromStdString(it->error));
        auto* item = new QListWidgetItem(expression + QString("\n") + result, m_historyList);
        item->setData(Qt::UserRole, expression);
        item->setToolTip(expression + QString("\n") + result);
        item->setSizeHint(QSize(0, 48));
    }
    const bool empty = m_historyList->count() == 0;
    m_historyEmpty->setVisible(empty);
    m_historyList->setVisible(!empty);
    m_historyClear->setEnabled(!empty);
}

QString MathSolverPage::resultText() const { return m_result->text(); }

void MathSolverPage::toggleAngleMode(bool degrees) {
    m_calc.setAngleUnit(degrees ? AngleUnit::Degrees : AngleUnit::Radians);
    m_angleButton->setText(degrees ? "DEG" : "RAD");
}

} // namespace phy104
