#include "StudyHubPage.h"

#include "AppContext.h"
#include "Components.h"
#include "DiagramsPage.h"
#include "JsonRepositories.h"
#include "LanguageManager.h"
#include "LatexRenderer.h"
#include "ui_StudyHubPage.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSizePolicy>
#include <QStringList>
#include <QStackedWidget>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QVector>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <utility>

namespace phy104 {
namespace {

/// Build a compact local, extractive draft from the most informative sentences in a note.
QString extractNoteSummary(const QString& source) {
    const QStringList sentences = source.split(QRegularExpression(QStringLiteral("(?<=[.!?])\\s+|\\n+")), Qt::SkipEmptyParts);
    if (sentences.isEmpty()) return {};

    static const QSet<QString> stopWords = {
        "a", "about", "also", "an", "and", "are", "as", "at", "be", "because", "but", "by", "can",
        "does", "for", "from", "had", "has", "have", "in", "into", "is", "it", "its", "more", "most",
        "not", "of", "on", "or", "our", "only", "so", "that", "the", "their", "then", "there", "these",
        "this", "those", "to", "use", "used", "using", "was", "were", "when", "which", "while", "with",
        "you", "your"
    };
    static const QRegularExpression wordPattern(QStringLiteral("[\\p{L}\\p{N}]{3,}"));
    QHash<QString, int> frequency;
    for (const QString& sentence : sentences) {
        auto words = wordPattern.globalMatch(sentence.toLower());
        while (words.hasNext()) {
            const QString word = words.next().captured();
            if (!stopWords.contains(word)) ++frequency[word];
        }
    }

    struct ScoredSentence { qsizetype index; double score; };
    QVector<ScoredSentence> ranked;
    ranked.reserve(sentences.size());
    for (qsizetype i = 0; i < sentences.size(); ++i) {
        int contentWords = 0;
        double score = 0.0;
        auto words = wordPattern.globalMatch(sentences.at(i).toLower());
        while (words.hasNext()) {
            const QString word = words.next().captured();
            const int count = frequency.value(word);
            if (count > 0) {
                ++contentWords;
                score += 1.0 + std::log(static_cast<double>(count));
            }
        }
        if (contentWords > 0) score /= std::sqrt(static_cast<double>(contentWords));
        if (i == 0) score *= 1.05;                 // slight preference for an opening definition
        ranked.push_back({i, score});
    }
    std::stable_sort(ranked.begin(), ranked.end(), [](const ScoredSentence& a, const ScoredSentence& b) {
        return a.score > b.score;
    });

    const qsizetype sentenceCount = sentences.size();
    const qsizetype wanted = std::min<qsizetype>(3, std::max<qsizetype>(1,
        static_cast<qsizetype>(std::ceil(static_cast<double>(sentenceCount) * 0.28))));
    QSet<qsizetype> selected;
    for (qsizetype i = 0; i < wanted && i < ranked.size(); ++i) selected.insert(ranked[i].index);
    QStringList summary;
    for (qsizetype i = 0; i < sentenceCount; ++i)
        if (selected.contains(i)) summary.push_back(sentences.at(i).trimmed());
    return summary.join(" ");
}

} // namespace

StudyHubPage::StudyHubPage(std::vector<StudyTopic> topics, std::vector<StudyNote> studyNotes,
                           AppContext* context, DiagramsPage* opticalSetups, QString notesFilePath, QWidget* parent)
    : BasePage("Study Hub", "STUDY / Chapters, JSON notes and optical setups", context, parent),
      m_topics(std::move(topics)), m_studyMaterials(std::move(studyNotes)),
      m_notesFilePath(std::move(notesFilePath)) {
    // ---------------------------------------------------------------- Designer-defined Study Hub sections; chapter and note content is data-driven
    auto* studyHubFormHost = new QWidget;
    Ui::StudyHubPage ui;
    ui.setupUi(studyHubFormHost);
    content()->addWidget(studyHubFormHost, 1);

    m_sections = new ModeTabs({"Chapters", "Notes", "Optical Setups"});
    m_sections->setObjectName("studyHubTabs");
    ui.studyHubTabsLayout->addWidget(m_sections);
    m_sectionStack = ui.studyHubSections;

    ui.chaptersPanel->setObjectName("panel");
    ui.chaptersHeader->setObjectName("panelHeader");
    ui.chaptersBody->setObjectName("panelBody");
    ui.chaptersTitle->setObjectName("panelTitle");
    ui.chaptersMeta->setObjectName("panelMeta");
    ui.chaptersMeta->setText(QString::number(m_topics.size()));
    m_list = ui.studyTopics;
    for (const StudyTopic& topic : m_topics) {
        auto* item = new QListWidgetItem(QString::fromStdString(topic.title));
        item->setToolTip(QString::fromStdString(topic.chapter));
        m_list->addItem(item);
    }

    ui.summaryPanel->setObjectName("panel");
    ui.summaryHeader->setObjectName("panelHeader");
    ui.summaryBody->setObjectName("panelBody");
    ui.summaryPanelTitle->setObjectName("panelTitle");
    m_chapter = ui.chapterLabel;
    m_chapter->setObjectName("caption");
    m_title = ui.topicTitle;
    m_title->setObjectName("studyTitle");
    m_summary = ui.topicSummary;
    m_summary->setObjectName("studyBody");

    ui.rulesPanel->setObjectName("panel");
    ui.rulesHeader->setObjectName("panelHeader");
    ui.rulesBody->setObjectName("panelBody");
    ui.rulesTitle->setObjectName("panelTitle");
    ui.rulesMeta->setObjectName("panelMeta");
    m_rulesPanelTitle = ui.rulesTitle;
    ui.rulesScroll->viewport()->setAutoFillBackground(false);
    m_rules = ui.rulesContentLayout;

    ui.constantsPanel->setObjectName("panel");
    ui.constantsHeader->setObjectName("panelHeader");
    ui.constantsBody->setObjectName("panelBody");
    ui.constantsTitle->setObjectName("panelTitle");
    ui.constantsMeta->setObjectName("panelMeta");
    m_constants = ui.chapterConstants;
    m_constants->setHorizontalHeaderLabels({"Name", "Value"});
    m_constants->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_constants->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_constants->horizontalHeader()->setStretchLastSection(true);
    m_constants->verticalHeader()->hide();

    m_noteViews = new ModeTabs({"Study material", "My notes"});
    m_noteViews->setObjectName("studyHubNoteTabs");
    ui.studyHubNoteTabsLayout->addWidget(m_noteViews);
    m_noteStack = ui.studyHubNoteViews;

    ui.studyMaterialsPanel->setObjectName("panel");
    ui.studyMaterialsHeader->setObjectName("panelHeader");
    ui.studyMaterialsBody->setObjectName("panelBody");
    ui.studyMaterialsTitle->setObjectName("panelTitle");
    ui.studyMaterialsMeta->setObjectName("panelMeta");
    m_studyMaterialListMeta = ui.studyMaterialsMeta;
    m_studyMaterialListMeta->setText(QString::number(m_studyMaterials.size()));
    m_studyMaterialSearch = ui.studyMaterialSearch;
    m_studyMaterialSearch->setObjectName("studyMaterialSearch");
    m_studyMaterialChapterFilter = ui.studyMaterialChapterFilter;
    m_studyMaterialChapterFilter->setObjectName("studyMaterialChapterFilter");
    m_studyMaterialChapterFilter->addItem("All chapters", QString());
    QSet<QString> studyChapters;
    for (const StudyNote& note : m_studyMaterials) {
        const QString chapterName = QString::fromStdString(note.chapter);
        if (studyChapters.contains(chapterName)) continue;
        studyChapters.insert(chapterName);
        m_studyMaterialChapterFilter->addItem(chapterName, chapterName);
    }
    m_studyMaterialList = ui.studyMaterialList;
    m_studyMaterialList->setObjectName("studyMaterialList");

    ui.studyMaterialDetailPanel->setObjectName("panel");
    ui.studyMaterialDetailHeader->setObjectName("panelHeader");
    ui.studyMaterialDetailPanelBody->setObjectName("panelBody");
    ui.studyMaterialDetailPanelTitle->setObjectName("panelTitle");
    m_studyMaterialMeta = ui.studyMaterialMeta;
    m_studyMaterialTitle = ui.studyMaterialTitle;
    ui.studyMaterialDetailScroll->viewport()->setAutoFillBackground(false);
    ui.studyMaterialDetailContents->setObjectName("studyMaterialDetailBody");
    m_studyMaterialBody = ui.studyMaterialDetailLayout;

    ui.personalNotesPanel->setObjectName("panel");
    ui.personalNotesHeader->setObjectName("panelHeader");
    ui.personalNotesBody->setObjectName("panelBody");
    ui.personalNotesTitle->setObjectName("panelTitle");
    ui.personalNotesMeta->setObjectName("panelMeta");
    ui.noteChapterLabel->setObjectName("fieldLabel");
    ui.notesEditorLabel->setObjectName("fieldLabel");
    ui.noteSummaryLabel->setObjectName("fieldLabel");
    m_noteChapter = ui.studyHubNoteChapter;
    m_noteChapter->setObjectName("studyHubNoteChapter");
    for (const StudyTopic& topic : m_topics)
        m_noteChapter->addItem(QString::fromStdString(topic.title), noteKey(topic));
    m_notesEditor = ui.studyHubNoteEditor;
    m_notesEditor->setObjectName("studyHubNoteEditor");
    m_noteSummaryEditor = ui.studyHubNoteSummaryEditor;
    m_noteSummaryEditor->setObjectName("studyHubNoteSummaryEditor");
    m_summarizeNoteButton = ui.studyHubSummarizeNote;
    m_summarizeNoteButton->setObjectName("studyHubSummarizeNote");
    m_summarizeNoteButton->setEnabled(false);
    auto* noteEditors = ui.studyHubNoteSplit;
    noteEditors->setObjectName("studyHubNoteSplit");
    noteEditors->setStretchFactor(0, 2);
    noteEditors->setStretchFactor(1, 1);
    m_noteStatus = ui.studyHubNoteStatus;
    m_noteStatus->setObjectName("caption");

    if (!loadStudyHubNotes(m_notesFilePath, m_studyNotes, &m_notesLoadError)) {
        m_notesLoadFailed = true;
        m_noteChapter->setEnabled(false);
        m_notesEditor->setReadOnly(true);
        m_noteSummaryEditor->setReadOnly(true);
        m_summarizeNoteButton->setEnabled(false);
    }
    m_noteSaveTimer = new QTimer(this);
    m_noteSaveTimer->setSingleShot(true);
    m_noteSaveTimer->setInterval(650);
    connect(m_noteSaveTimer, &QTimer::timeout, this, &StudyHubPage::saveCurrentNote);
    connect(m_noteChapter, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        flushStudyNotes();
        loadCurrentNote();
    });
    const auto queueStudyNoteSave = [this] {
        if (m_loadingNote || m_notesLoadFailed) return;
        m_noteStatus->setText(QString(" ") + QFileInfo(m_notesFilePath).fileName());
        m_noteSaveTimer->start();
    };
    connect(m_notesEditor, &QPlainTextEdit::textChanged, this, queueStudyNoteSave);
    connect(m_noteSummaryEditor, &QPlainTextEdit::textChanged, this, queueStudyNoteSave);
    connect(m_notesEditor, &QPlainTextEdit::textChanged, this, [this] {
        m_summarizeNoteButton->setEnabled(!m_notesLoadFailed && !m_notesEditor->toPlainText().trimmed().isEmpty());
    });
    connect(m_summarizeNoteButton, &QPushButton::clicked, this, [this] {
        m_noteSummaryEditor->setPlainText(extractNoteSummary(m_notesEditor->toPlainText()));
    });
    loadCurrentNote();
    if (m_notesLoadFailed)
        m_noteStatus->setText(QString(" ") + m_notesLoadError);

    connect(m_noteViews, &ModeTabs::currentChanged, m_noteStack, &QStackedWidget::setCurrentIndex);
    connect(m_studyMaterialSearch, &QLineEdit::textChanged, this, [this](const QString&) { filterStudyMaterials(); });
    connect(m_studyMaterialChapterFilter, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { filterStudyMaterials(); });
    connect(m_studyMaterialList, &QListWidget::currentRowChanged, this, &StudyHubPage::showStudyMaterial);
    filterStudyMaterials();

    // ---------------------------------------------------------------- full optical setup gallery
    ui.opticalSetupsLayout->addWidget(opticalSetups);

    connect(m_sections, &ModeTabs::currentChanged, m_sectionStack, &QStackedWidget::setCurrentIndex);
    connect(m_list, &QListWidget::currentRowChanged, this, &StudyHubPage::selectTopic);
    if (!m_topics.empty()) m_list->setCurrentRow(0);
}

StudyHubPage::~StudyHubPage() { flushStudyNotes(); }

QString StudyHubPage::noteKey(const StudyTopic& topic) const {
    if (!topic.id.empty()) return QString::fromStdString(topic.id);
    if (!topic.chapter.empty()) return QString::fromStdString(topic.chapter);
    return QString::fromStdString(topic.title);
}

void StudyHubPage::filterStudyMaterials() {
    if (!m_studyMaterialList) return;
    const QString query = m_studyMaterialSearch->text().trimmed().toCaseFolded();
    const QString selectedChapter = m_studyMaterialChapterFilter->currentData().toString();
    const int previousIndex = m_studyMaterialList->currentItem()
                                  ? m_studyMaterialList->currentItem()->data(Qt::UserRole).toInt() : -1;
    const QSignalBlocker blocker(m_studyMaterialList);
    m_studyMaterialList->clear();

    int selectedRow = -1;
    for (std::size_t i = 0; i < m_studyMaterials.size(); ++i) {
        const StudyNote& note = m_studyMaterials[i];
        const QString chapter = QString::fromStdString(note.chapter);
        if (!selectedChapter.isEmpty() && chapter != selectedChapter) continue;

        QStringList searchable;
        const auto addSearchTerm = [&searchable](const QString& source) {
            searchable.push_back(source);
            const QString localized = LanguageManager::instance().text(source);
            if (localized != source) searchable.push_back(localized);
        };
        addSearchTerm(QString::fromStdString(note.id));
        addSearchTerm(chapter);
        addSearchTerm(QString::fromStdString(note.type));
        addSearchTerm(QString::fromStdString(note.title));
        for (const std::string& line : note.content) addSearchTerm(QString::fromStdString(line));
        for (const std::string& equation : note.equations) searchable.push_back(QString::fromStdString(equation));
        if (!query.isEmpty() && !searchable.join(QLatin1Char(' ')).toCaseFolded().contains(query)) continue;

        auto* item = new QListWidgetItem(QString::fromStdString(note.title));
        item->setData(Qt::UserRole, static_cast<int>(i));
        item->setToolTip(chapter + QString(" ") + QString::fromStdString(note.type));
        const int row = m_studyMaterialList->count();
        m_studyMaterialList->addItem(item);
        if (static_cast<int>(i) == previousIndex) selectedRow = row;
    }

    if (selectedRow < 0 && m_studyMaterialList->count() > 0) selectedRow = 0;
    m_studyMaterialList->setCurrentRow(selectedRow);
    m_studyMaterialListMeta->setText(QStringLiteral("%1 / %2")
                                          .arg(m_studyMaterialList->count())
                                          .arg(static_cast<qulonglong>(m_studyMaterials.size())));
    LanguageManager::instance().apply(m_studyMaterialList);
    showStudyMaterial(selectedRow);
}

void StudyHubPage::showStudyMaterial(int row) {
    clearLayout(m_studyMaterialBody);
    if (row < 0 || row >= m_studyMaterialList->count()) {
        m_studyMaterialMeta->clear();
        m_studyMaterialTitle->setText(LanguageManager::instance().text("No study materials match your filters."));
        return;
    }

    const int index = m_studyMaterialList->item(row)->data(Qt::UserRole).toInt();
    if (index < 0 || index >= static_cast<int>(m_studyMaterials.size())) return;
    const StudyNote& note = m_studyMaterials[static_cast<std::size_t>(index)];
    const QString chapter = QString::fromStdString(note.chapter);
    const QString type = QString::fromStdString(note.type);
    m_studyMaterialMeta->setText(chapter + QStringLiteral("  ·  ") + type.toUpper() + QStringLiteral("  ·  ") +
                                 QString::fromStdString(note.id));
    m_studyMaterialTitle->setText(QString::fromStdString(note.title));

    auto* pointsHeading = new QLabel(LanguageManager::instance().text("Key points"));
    pointsHeading->setObjectName("sectionTitle");
    m_studyMaterialBody->addWidget(pointsHeading);
    for (const std::string& line : note.content) {
        const QString text = QString::fromStdString(line);
        auto* point = new QLabel(QStringLiteral("•  ") + text);
        point->setObjectName("studyMaterialPoint");
        point->setProperty("studyNoteContent", text);
        point->setWordWrap(true);
        point->setAlignment(Qt::AlignTop | Qt::AlignLeading);
        point->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_studyMaterialBody->addWidget(point);
    }

    if (!note.equations.empty()) {
        auto* divider = new QFrame;
        divider->setObjectName("studyMaterialEquationDivider");
        divider->setFrameShape(QFrame::HLine);
        m_studyMaterialBody->addWidget(divider);
        auto* equationsHeading = new QLabel(LanguageManager::instance().text("Equations"));
        equationsHeading->setObjectName("sectionTitle");
        m_studyMaterialBody->addWidget(equationsHeading);
        for (const std::string& source : note.equations) {
            auto* equationScroll = new QScrollArea;
            equationScroll->setObjectName("studyMaterialEquationScroll");
            equationScroll->setWidgetResizable(false);
            equationScroll->setMinimumWidth(0);
            equationScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            equationScroll->setFrameShape(QFrame::NoFrame);
            equationScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            equationScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

            auto* equation = new LatexWidget;
            equation->setObjectName("studyMaterialEquation");
            equation->setProperty("rawEquation", QString::fromStdString(source));
            equation->setAlignLeft(true);
            equation->setPointSize(15.0);
            equation->setLatex(QString::fromStdString(source));
            const QSize equationSize = equation->sizeHint();
            equation->setFixedSize(equationSize);
            equationScroll->setFixedHeight(qMax(58, equationSize.height() + 20));
            equationScroll->setWidget(equation);
            m_studyMaterialBody->addWidget(equationScroll);
        }
    }
    m_studyMaterialBody->addStretch(1);
    LanguageManager::instance().apply(m_studyMaterialMeta);
    LanguageManager::instance().apply(m_studyMaterialTitle);
    LanguageManager::instance().apply(m_studyMaterialBody->parentWidget());
}

void StudyHubPage::loadCurrentNote() {
    if (!m_notesEditor || !m_noteSummaryEditor) return;
    const QString key = m_noteChapter ? m_noteChapter->currentData().toString() : QString();
    const StudyHubNote note = m_studyNotes.value(key);
    m_loadingNote = true;
    m_notesEditor->setPlainText(QString::fromStdString(note.text));
    m_noteSummaryEditor->setPlainText(QString::fromStdString(note.summary));
    m_loadedNoteKey = key;
    m_loadingNote = false;
    if (key.isEmpty()) m_noteStatus->setText("Select a chapter to write its notes.");
    else if (m_notesDirty) m_noteStatus->setText(QString(" ") + QFileInfo(m_notesFilePath).fileName());
    else m_noteStatus->setText(QString(" ") + QFileInfo(m_notesFilePath).fileName());
}

void StudyHubPage::saveCurrentNote() {
    if (m_notesLoadFailed || !m_notesEditor || !m_noteSummaryEditor || m_loadedNoteKey.isEmpty()) return;
    const StudyHubNote note{m_notesEditor->toPlainText().toStdString(), m_noteSummaryEditor->toPlainText().toStdString()};
    if (note.text.empty() && note.summary.empty()) {
        if (m_studyNotes.remove(m_loadedNoteKey) > 0) m_notesDirty = true;
    } else if (!m_studyNotes.contains(m_loadedNoteKey) || m_studyNotes.value(m_loadedNoteKey) != note) {
        m_studyNotes.insert(m_loadedNoteKey, note);
        m_notesDirty = true;
    }
    if (!m_notesDirty) return;
    QString error;
    if (saveStudyHubNotes(m_notesFilePath, m_studyNotes, &error)) {
        m_notesDirty = false;
        m_noteStatus->setText(QString("Saved notes ") + QFileInfo(m_notesFilePath).fileName());
    } else {
        m_noteStatus->setText(QString(" ") + error);
        emit m_ctx->message(QString(" ") + error);
    }
}

void StudyHubPage::flushStudyNotes() {
    if (m_noteSaveTimer && m_noteSaveTimer->isActive()) m_noteSaveTimer->stop();
    saveCurrentNote();
}

int StudyHubPage::currentSection() const { return m_sections->current(); }

void StudyHubPage::selectSection(int index) {
    if (index < 0 || index >= m_sectionStack->count()) return;
    m_sections->setCurrent(index);             // ModeTabs does not emit for programmatic changes
    m_sectionStack->setCurrentIndex(index);
}

void StudyHubPage::selectTopic(int row) {
    if (row < 0 || row >= int(m_topics.size())) return;
    const StudyTopic& topic = m_topics[std::size_t(row)];

    m_chapter->setText(QString::fromStdString(topic.chapter).toUpper());
    m_title->setText(QString::fromStdString(topic.title));
    m_summary->setText(QString::fromStdString(topic.summary));
    m_rulesPanelTitle->setText(QString(" ") + QString::fromStdString(topic.title));

    clearLayout(m_rules);
    int index = 0;
    for (const StudyRule& rule : topic.rules) {
        if (index > 0) {
            auto* line = new QFrame;
            line->setObjectName("ruleLine");
            line->setFrameShape(QFrame::HLine);
            m_rules->addWidget(line);
        }
        auto* formula = new LatexWidget;
        formula->setAlignLeft(true);
        formula->setLatex(LatexRenderer::fromPlainMath(QString::fromStdString(rule.formula)));
        m_rules->addWidget(formula);
        if (!rule.explain.empty()) {
            auto* explain = new QLabel(QString::fromStdString(rule.explain));
            explain->setObjectName("studyBody");
            explain->setWordWrap(true);
            m_rules->addWidget(explain);
        }
        ++index;
    }
    m_rules->addStretch(1);

    m_constants->setRowCount(int(topic.constants.size()));
    for (int r = 0; r < int(topic.constants.size()); ++r) {
        m_constants->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(topic.constants[std::size_t(r)].name)));
        m_constants->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(topic.constants[std::size_t(r)].value)));
    }
    m_constants->resizeColumnToContents(0);
}

} // namespace phy104
