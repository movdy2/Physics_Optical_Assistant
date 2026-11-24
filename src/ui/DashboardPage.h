#pragma once
#include "BasePage.h"

#include <QList>
#include <QMap>

class QCheckBox;
class QComboBox;
class QFrame;
class QLabel;
class QPushButton;
class QToolButton;
class QVBoxLayout;

namespace phy104 {

class LineChart;
struct ChartSeries;
struct DataSeries;
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
    void refreshComparison();                             // rebuild the run-comparison chart panel
    void deleteSelectedRun();
    int pageForExperiment(const std::string& name) const; // experiment name -> MainWindow page index
    ChartSeries seriesToChart(const DataSeries& s, const QColor& color) const;

    int m_notesPage = 5;
    int m_exportPage = 5;
    int m_selectedRunId = -1;                             // id of the currently highlighted log row

    QToolButton* m_browseExperiments = nullptr;
    QLabel* m_savedValue;
    QLabel* m_seriesValue;
    QList<QLabel*> m_results;          // "last result" line of each experiment card
    QList<QFrame*> m_featuredCards;
    QLabel* m_lastSaved;

    // activity log sidebar
    QVBoxLayout* m_logLayout = nullptr;
    QPushButton* m_clearLog = nullptr;
    QPushButton* m_deleteRun = nullptr;

    // run comparison chart (center column)
    QComboBox* m_comparisonFilter = nullptr;
    QVBoxLayout* m_runListLayout = nullptr;
    LineChart* m_comparisonChart = nullptr;
    QMap<int, QCheckBox*> m_runCheckboxes;                // run id -> checkbox
};

} // namespace phy104
