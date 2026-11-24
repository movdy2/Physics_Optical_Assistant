#pragma once
#include "BasePage.h"
#include "StudyHub.h"

#include <QMap>
#include <QString>
#include <vector>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTableWidget;
class QTimer;
class QVBoxLayout;

namespace phy104 {

class DiagramsPage;
class ModeTabs;

/// Study Hub: chapter guides, a structured study-note catalog, autosaved personal notes and optical setups.
class StudyHubPage : public BasePage {
    Q_OBJECT
public:
    explicit StudyHubPage(std::vector<StudyTopic> topics, std::vector<StudyNote> studyNotes,
                          AppContext* context, DiagramsPage* opticalSetups, QString notesFilePath,
                          QWidget* parent = nullptr);
    ~StudyHubPage() override;

    QListWidget* topicList() const { return m_list; }
    QTableWidget* constantTable() const { return m_constants; }              // constants for the selected chapter
    QLabel* summaryLabel() const { return m_summary; }
    QPlainTextEdit* notesEditor() const { return m_notesEditor; }
    QPlainTextEdit* noteSummaryEditor() const { return m_noteSummaryEditor; }
    QPushButton* summarizeNoteButton() const { return m_summarizeNoteButton; }
    QString notesFilePath() const { return m_notesFilePath; }
    void flushStudyNotes();
    int topicCount() const { return int(m_topics.size()); }
    int currentSection() const;
    void selectSection(int index);

public slots:
    void selectTopic(int row);

private:
    std::vector<StudyTopic> m_topics;
    std::vector<StudyNote> m_studyMaterials;
    ModeTabs* m_sections = nullptr;
    QStackedWidget* m_sectionStack = nullptr;
    ModeTabs* m_noteViews = nullptr;
    QStackedWidget* m_noteStack = nullptr;
    QLineEdit* m_studyMaterialSearch = nullptr;
    QComboBox* m_studyMaterialChapterFilter = nullptr;
    QListWidget* m_studyMaterialList = nullptr;
    QLabel* m_studyMaterialListMeta = nullptr;
    QLabel* m_studyMaterialMeta = nullptr;
    QLabel* m_studyMaterialTitle = nullptr;
    QVBoxLayout* m_studyMaterialBody = nullptr;
    QListWidget* m_list = nullptr;
    QLabel* m_chapter = nullptr;
    QLabel* m_title = nullptr;
    QLabel* m_summary = nullptr;
    QVBoxLayout* m_rules = nullptr;          // filled again for every chapter
    QTableWidget* m_constants = nullptr;
    QComboBox* m_noteChapter = nullptr;
    QPlainTextEdit* m_notesEditor = nullptr;
    QPlainTextEdit* m_noteSummaryEditor = nullptr;
    QPushButton* m_summarizeNoteButton = nullptr;
    QLabel* m_noteStatus = nullptr;
    QTimer* m_noteSaveTimer = nullptr;
    QString m_notesFilePath;
    QString m_loadedNoteKey;
    QString m_notesLoadError;
    QMap<QString, StudyHubNote> m_studyNotes;
    bool m_notesLoadFailed = false;
    bool m_loadingNote = false;
    bool m_notesDirty = false;
    QLabel* m_rulesPanelTitle = nullptr;

    QString noteKey(const StudyTopic& topic) const;
    void filterStudyMaterials();
    void showStudyMaterial(int row);
    void loadCurrentNote();
    void saveCurrentNote();
};

} // namespace phy104
