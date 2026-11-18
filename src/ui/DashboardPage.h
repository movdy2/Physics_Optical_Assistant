#pragma once
#include "BasePage.h"

class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QToolButton;
class QVBoxLayout;

namespace phy104 {

class LineChart;
struct RunRecord;

class DashboardPage : public BasePage {
    Q_OBJECT
public:
    /// Page indexes the cards open (see MainWindow order).
    DashboardPage(AppContext* context, QWidget* parent = nullptr);

    void setNotesPage(int index) { m_notesPage = index; }
    void setExportPage(int index) { m_exportPage = index; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void refresh();
    void refreshLog();                                    // rebuild the activity-log sidebar rows
    void deleteSelectedRun();
    int pageForExperiment(const std::string& name) const; // experiment name -> MainWindow page index

    int m_notesPage = 5;
    int m_exportPage = 5;
    int m_selectedRunId = -1;                             // id of the currently highlighted log row

    QToolButton* m_browseExperiments = nullptr;
    QLabel* m_savedValue;
    QLabel* m_seriesValue;
    QList<QLabel*> m_results;          // "last result" line of each experiment card
    QLabel* m_lastSaved;

    // activity log sidebar
    QScrollArea* m_logScroll = nullptr;
    QWidget* m_logList = nullptr;
    QVBoxLayout* m_logLayout = nullptr;
    QPushButton* m_clearLog = nullptr;
    QPushButton* m_deleteRun = nullptr;
};

} // namespace phy104
