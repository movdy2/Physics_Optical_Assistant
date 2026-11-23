#pragma once
#include "BasePage.h"
#include "MathCalculator.h"

#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace phy104 {
class AppContext;
class CalculatorKeypad;

/// Compact scientific calculator with advanced functions, optics units, and reusable Ans/history.
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
    void refreshHistory();

private:
    MathCalculator m_calc;
    QLineEdit* m_input = nullptr;
    QLabel* m_result = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_historyEmpty = nullptr;
    QPushButton* m_angleButton = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_historyClear = nullptr;
    QListWidget* m_historyList = nullptr;
    CalculatorKeypad* m_keypad = nullptr;
};

} // namespace phy104
