#pragma once
#include "BasePage.h"
#include "MathCalculator.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

namespace phy104 {
class AppContext;
class CalculatorKeypad;

/// Clean, free-math mode: enter an expression, get a result, and reuse recent answers with Ans.
class MathSolverPage : public BasePage {
    Q_OBJECT
public:
    explicit MathSolverPage(AppContext* context, QWidget* parent = nullptr);

    /// The current result as plain text, e.g. "0.5236" or "1.5708"
    QString resultText() const;

private slots:
    void calculate();
    void clearAll();
    void toggleAngleMode(bool degrees);
    void onCopyResult();

private:
    MathCalculator m_calc;
    QLineEdit* m_input = nullptr;
    QLabel* m_result = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_angleButton = nullptr;
    QPushButton* m_copyBtn = nullptr;
    CalculatorKeypad* m_keypad = nullptr;
};

} // namespace phy104
