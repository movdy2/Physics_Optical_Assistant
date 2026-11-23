#pragma once
#include "FormulaEngine.h"
#include "FormulaManager.h"
#include "SolverEngine.h"
#include "BasePage.h"
#include <map>
#include <memory>
#include <QStringList>

class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QSplitter;

namespace phy104 {

class LatexWidget;

/// Formula Solver page.
///  left : search box, chapter filter, recent formulas, paged list of formula cards
///  right: LaTeX formula, input fields, status line, big answer, copy-to-clipboard action.
class SolverPage : public BasePage {
    Q_OBJECT
public:
    SolverPage(std::shared_ptr<FormulaEngine> engine, std::shared_ptr<FormulaManager> manager,
               std::shared_ptr<SolverEngine> solver, AppContext* context, QWidget* parent = nullptr);

public slots:
    /// Selects a formula by id (also makes sure it is visible in the list).
    void selectFormula(const QString& id);

private slots:
    void onSearchChanged();
    void onChapterChanged();
    void onPageSizeChanged();
    void onItemSelected();
    void prevPage();
    void nextPage();
    void onSolve();
    void onClear();
    void onRecentFormula(const QString& id);

private:
    void refreshList(bool resetPage);
    void refreshChapterCombo();
    void refreshRecent();
    void showFormula(const Formula& formula);
    void setStatus(const QString& kind, const QString& text);   // kind: Ok | Warn | Error | "" (hide)
    void updateVariableHelp();                                   // rebuild the variable help panel
    void addToRecent(const std::string& id);
    void showEmptyState(bool show);

    std::shared_ptr<FormulaEngine> m_engine;
    std::shared_ptr<FormulaManager> m_manager;
    std::shared_ptr<SolverEngine> m_solver;
    const Formula* m_current = nullptr;

    // browser (left)
    QLineEdit* m_search;
    QComboBox* m_chapter;
    QComboBox* m_pageSize;
    QListWidget* m_list;
    QListWidget* m_recentList;                                   // recently opened formulas
    QPushButton* m_prev;
    QPushButton* m_next;
    QLabel* m_pageLabel;
    QLabel* m_emptyState;                                        // shown when no formulas match
    std::vector<const Formula*> m_results;
    int m_pageIndex = 0;
    QStringList m_recent;                                        // recently opened formula ids

    // solver (right)
    QSplitter* m_splitter;
    QLabel* m_formulaName;
    QLabel* m_formulaChapter;
    LatexWidget* m_formulaLatex;
    QFormLayout* m_form;
    std::map<std::string, QLineEdit*> m_edits;
    QLabel* m_status;
    LatexWidget* m_answer;
};

} // namespace phy104