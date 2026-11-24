#include "NotesPage.h"
#include "AppContext.h"
#include "Components.h"
#include "MarkdownWorkspace.h"
#include "ScientificKeyboard.h"
#include "ui_NotesPage.h"

#include <QToolButton>
#include <utility>

namespace phy104 {

NotesPage::NotesPage(AppContext* ctx, std::vector<ScientificPage> keyboardPages, QWidget* parent)
    : BasePage("Notes", "MARKDOWN STUDY WORKSPACE / Write, edit and preview your lab notes", ctx, parent) {
    // One panel across the whole window: the editor, its toolbar and the live preview, nothing else.
    m_notesPanel = new Panel("Session notes", QString(" ") + ctx->lastSavedTime());
    auto* notesContent = new QWidget;
    Ui::NotesPage form;
    form.setupUi(notesContent);
    m_keyboardToggle = form.notesKeyboard;

    m_notes = new MarkdownWorkspace;
    const QString saved = QString::fromStdString(ctx->log.notes());
    const bool showingStarter = saved.isEmpty();
    const QString initialNotes = showingStarter ? MarkdownWorkspace::starterNotes() : saved;
    m_notes->setMarkdown(initialNotes);
    if (showingStarter) {
        m_notes->editor()->setProperty("i18nBuiltinSource", initialNotes);
        m_notes->editor()->setProperty("i18nBuiltinContent", true);
    }
    form.editorSlotLayout->addWidget(m_notes, 1);

    // the same scientific keyboard as the Equation Lab, docked under the note and writing LaTeX into it
    m_keyboard = new ScientificKeyboard(std::move(keyboardPages), this);
    m_keyboard->setTarget(m_notes->editor());
    m_keyboard->setFixedSize(600, 330);
    m_keyboard->hide();
    form.keyboardSlotLayout->addWidget(m_keyboard, 0, Qt::AlignLeft);

    m_notesPanel->body()->setContentsMargins(12, 0, 12, 10);
    m_notesPanel->body()->addWidget(notesContent, 1);
    content()->addWidget(m_notesPanel, 1);

    connect(m_keyboardToggle, &QToolButton::toggled, this, &NotesPage::toggleKeyboard);
    connect(m_keyboard, &ScientificKeyboard::closeRequested, this, [this] { m_keyboardToggle->setChecked(false); });

    // autosave: the workspace reports its content ~0.7 s after the last keystroke; the context stores it in the
    // session log (and into every snapshot's notesContent) and writes the .phy file
    connect(m_notes, &MarkdownWorkspace::contentChanged, ctx, &AppContext::setNotes);
    connect(ctx, &AppContext::notesAppended, this, [this] { m_notes->setMarkdown(QString::fromStdString(m_ctx->log.notes())); });
    connect(ctx, &AppContext::notesSaved, this, [this] {
        const QString latest = QString::fromStdString(m_ctx->log.notes());
        if (m_notes->markdown() != latest) m_notes->setMarkdown(latest);   // keep the Study Hub editor in sync
        m_notesPanel->setMeta(QString(" ") + m_ctx->lastSavedTime());
        m_notes->setSavedText(QString(" ") + m_ctx->lastSavedTime());
    });
}

void NotesPage::toggleKeyboard(bool on) {
    m_keyboard->setVisible(on);
    m_keyboardToggle->setText(on ? "⌨ Keyboard · open" : "⌨ Keyboard");
    if (!on) return;
    // the keyboard inserts LaTeX tokens, so put the caret inside a formula:  $…$
    m_notes->editor()->insertSnippet("$%1$");
    m_keyboard->setTarget(m_notes->editor());
    m_keyboard->setFocus();
}

} // namespace phy104
