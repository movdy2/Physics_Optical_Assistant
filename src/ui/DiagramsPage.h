#pragma once
#include <QWidget>

class QComboBox;
class QLabel;
class QListWidget;

namespace phy104 {

class SvgDiagramView;

/// Gallery of reference setups for all eleven interactive experiment modules.
class DiagramsPage : public QWidget {
    Q_OBJECT
public:
    explicit DiagramsPage(QWidget* parent = nullptr);

private slots:
    void showDiagram(int row);
    void showDiagramView(int index);
    void exportPng();

private:
    SvgDiagramView* m_view = nullptr;
    QListWidget* m_list = nullptr;
    QComboBox* m_viewSelector = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_caption = nullptr;
    QWidget* m_equations = nullptr;
    int m_currentDiagram = -1;
};

} // namespace phy104
