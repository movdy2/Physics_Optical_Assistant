#pragma once

#include <QDialog>
#include <functional>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;

namespace phy104 {

class AppContext;

/// A short, non-modal lab protocol. Students can use the experiment while the guide stays open.
class GuidedLabDialog final : public QDialog {
    Q_OBJECT
public:
    explicit GuidedLabDialog(AppContext* context, std::function<void()> applyStarterValues,
                             QWidget* parent = nullptr);

private:
    void setStep(int step);
    void refreshResult();
    void addReportToNotes();

    AppContext* m_context = nullptr;
    std::function<void()> m_applyStarterValues;
    QStackedWidget* m_steps = nullptr;
    QLabel* m_stepLabel = nullptr;
    QLabel* m_result = nullptr;
    QLabel* m_predictionEcho = nullptr;
    QLabel* m_savedStatus = nullptr;
    QPlainTextEdit* m_prediction = nullptr;
    QPlainTextEdit* m_reflection = nullptr;
    QPushButton* m_back = nullptr;
    QPushButton* m_next = nullptr;
    QPushButton* m_addToNotes = nullptr;
    int m_currentStep = 0;
};

} // namespace phy104
