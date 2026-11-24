#include "Components.h"
#include "ExpressionEvaluator.h"
#include "Icons.h"
#include "LanguageManager.h"
#include "ThemeManager.h"
#include "ui_CalculatorKeypad.h"
#include "ui_ModeTabs.h"
#include "ui_NoticeCard.h"
#include "ui_Panel.h"
#include "ui_RegisterView.h"
#include "ui_ResultCard.h"
#include "ui_UnitField.h"

#include <QButtonGroup>
#include <QDialog>
#include <QEvent>
#include <QObject>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QStringList>
#include <cmath>

namespace phy104 {

void clearLayout(QLayout* layout) {
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* w = item->widget()) { w->hide(); w->deleteLater(); }
        else if (QLayout* child = item->layout()) clearLayout(child);   // a layout item IS the layout: delete it once, below
        delete item;
    }
}

QWidget* labeled(const QString& label, QWidget* field) {
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(4);
    auto* cap = new QLabel(label);
    cap->setObjectName("fieldLabel");
    cap->setMinimumWidth(0);
    cap->setMinimumHeight(0);
    cap->setWordWrap(true);
    l->addWidget(cap);
    l->addWidget(field);
    if (auto* unitField = qobject_cast<UnitField*>(field)) {           // inline error text under the field
        auto* err = new QLabel;
        err->setObjectName("fieldError");
        err->setWordWrap(true);
        err->hide();
        l->addWidget(err);
        QObject::connect(unitField, &UnitField::errorTextChanged, err, [err](const QString& msg) {
            err->setText(msg);
            err->setVisible(!msg.isEmpty());
        });
    }
    return w;
}

// ---------------------------------------------------------------------------- Panel
Panel::Panel(const QString& title, const QString& meta, QWidget* parent) : QFrame(parent) {
    Ui::Panel form;
    form.setupUi(this);
    m_title = form.panelTitle;
    m_meta = form.panelMeta;
    m_body = form.bodyLayout;
    m_title->setText(title);
    m_meta->setText(meta.toUpper());
}
QVBoxLayout* Panel::body() const {
    if (!m_body) {
        qWarning("Panel::body(): m_body was null, reinitializing lazily");
    auto* holder = new QWidget(const_cast<Panel*>(this));
    m_body = new QVBoxLayout(holder);
    m_body->setContentsMargins(12, 12, 12, 12);
    m_body->setSpacing(12);
        if (auto* root = layout())
            root->addWidget(holder);
    }
    return m_body;
}

void Panel::setMeta(const QString& meta, const QString& tooltip) {
    m_meta->setText(meta.toUpper());
    m_meta->setToolTip(tooltip);
}
void Panel::setTitle(const QString& title) { m_title->setText(title); m_title->setWordWrap(true); m_title->setMinimumWidth(0); }

// ---------------------------------------------------------------------------- UnitField
UnitField::UnitField(const QString& unit, QWidget* parent) : QFrame(parent) {
    Ui::UnitField form;
    form.setupUi(this);
    m_edit = form.numberEdit;
    m_unit = form.unitLabel;
    m_arrows = form.spinnerControls;
    m_pasteBtn = form.pasteButton;
    m_unit->setObjectName("unit");
    m_unit->setText(unit);

    m_edit->installEventFilter(this);
    installEventFilter(this);
    m_arrows->hide();
    m_pasteBtn->setCursor(Qt::PointingHandCursor);
    m_pasteBtn->setEnabled(false);

    const auto connectArrow = [this](QToolButton* button, const char* icon, int direction) {
        button->setCursor(Qt::PointingHandCursor);
        button->setProperty("iconName", icon);
        connect(button, &QToolButton::clicked, this, [this, direction] { step(direction); });
    };
    connectArrow(form.spinUp, "chevron-up", +1);
    connectArrow(form.spinDown, "chevron-down", -1);

    // Paste button remains available to other UnitField users; dense screens can hide it.
    connect(m_pasteBtn, &QPushButton::clicked, this, [this] {
        QClipboard* cb = QApplication::clipboard();
        m_edit->insert(cb->text());
        m_edit->setFocus();
        emit edited();
    });
    connect(QApplication::clipboard(), &QClipboard::changed, this, [this] {
        m_pasteBtn->setEnabled(!QApplication::clipboard()->text().isEmpty());
    });
    m_pasteBtn->setEnabled(!QApplication::clipboard()->text().isEmpty());

    m_liveTimer = new QTimer(this);                  // live update while typing, after a short pause
    m_liveTimer->setSingleShot(true);
    m_liveTimer->setInterval(350);
    connect(m_liveTimer, &QTimer::timeout, this, &UnitField::edited);
    connect(m_edit, &QLineEdit::textEdited, m_liveTimer, qOverload<>(&QTimer::start));
    connect(m_edit, &QLineEdit::editingFinished, this, [this] {
        if (m_liveTimer->isActive()) { m_liveTimer->stop(); emit edited(); }
    });

    auto refreshIcons = [this] {
        const QColor c = ThemeManager::instance().theme().dim;
        const auto arrows = m_arrows->findChildren<QToolButton*>();
        for (auto* b : arrows)
            b->setIcon(Icons::get(b->property("iconName").toString(), c, ThemeManager::instance().theme().amber, 12));
    };
    refreshIcons();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, refreshIcons);
}

double UnitField::value(bool* ok) const {
    QString t = m_edit->text().trimmed();
    t.replace(',', '.');
    bool good = false;
    double v = QLocale::c().toDouble(t, &good);                       // plain number: fast path
    if (!good && !t.isEmpty()) {                                      // expression: 0.5*mm, 1/3, sqrt(2), π ...
        try {
            const AngleUnit unit = m_unit->text() == "°" ? AngleUnit::Degrees : AngleUnit::Radians;
            v = ExpressionEvaluator::evaluate(t.toStdString(), {}, unit);
            good = std::isfinite(v);
        } catch (const std::exception&) {
            good = false;
        }
    }
    if (ok) *ok = good;
    return good ? v : 0.0;
}

void UnitField::setValue(double v, int decimals) {
    m_decimals = decimals;
    QString text = QString::number(v, 'f', decimals);
    if (m_plus && v > 0) text.prepend('+');
    m_edit->setText(text);
}
QString UnitField::text() const { return m_edit->text(); }
void UnitField::setText(const QString& t) { m_edit->setText(t); }
void UnitField::setUnit(const QString& u) { m_unit->setText(u); }
QString UnitField::unit() const { return m_unit->text(); }

void UnitField::setSpinner(bool on, double stepSize) {
    m_spinnerOn = on;
    m_step = stepSize;
    m_arrows->setVisible(on);
}

void UnitField::setPasteButtonVisible(bool visible) {
    m_pasteBtn->setVisible(visible);
}

void UnitField::step(int direction) {
    bool ok = false;
    const double v = value(&ok);
    m_liveTimer->stop();
    setValue((ok ? v : 0.0) + direction * m_step, m_decimals);
    emit edited();
}

void UnitField::setError(bool on, const QString& message) {
    const bool changed = on != m_error;
    m_error = on;
    setProperty("error", on);
    if (changed) repolish();
    emit errorTextChanged(on ? message : QString());
}

void UnitField::repolish() {
    style()->unpolish(this);
    style()->polish(this);
}

bool UnitField::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_edit) {
        if (event->type() == QEvent::FocusIn || event->type() == QEvent::FocusOut) {
            setProperty("focused", event->type() == QEvent::FocusIn);
            repolish();
        } else if (m_spinnerOn && event->type() == QEvent::KeyPress) {
            const int key = static_cast<QKeyEvent*>(event)->key();
            if (key == Qt::Key_Up || key == Qt::Key_Down) { step(key == Qt::Key_Up ? +1 : -1); return true; }
        }
    }
    if (m_spinnerOn && event->type() == QEvent::Wheel && (watched == m_edit || watched == this) && m_edit->hasFocus()) {
        step(static_cast<QWheelEvent*>(event)->angleDelta().y() > 0 ? +1 : -1);
        return true;
    }
    return QFrame::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------- ModeTabs
ModeTabs::ModeTabs(const QStringList& labels, QWidget* parent) : QWidget(parent), m_group(new QButtonGroup(this)) {
    Ui::ModeTabs form;
    form.setupUi(this);
    auto* l = form.tabsLayout;
    int id = 0;
    for (const QString& label : labels) {
        auto* b = new QPushButton(QString(label).replace("&", "&&"), this);
        b->setObjectName("modeTab");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        m_group->addButton(b, id++);
        l->addWidget(b, 1);
    }
    m_group->setExclusive(true);
    if (m_group->button(0)) m_group->button(0)->setChecked(true);
    connect(m_group, &QButtonGroup::idClicked, this, &ModeTabs::currentChanged);
}
int ModeTabs::current() const { return m_group->checkedId(); }
void ModeTabs::setCurrent(int index) {
    if (auto* b = m_group->button(index)) b->setChecked(true);
}

// ---------------------------------------------------------------------------- Result-card labels
namespace {
QString displayResultTitle(const std::string& source) {
    QString title = QString::fromStdString(source);
    if (title.compare(QStringLiteral("CALCULATION REGISTER"), Qt::CaseInsensitive) == 0)
        title = QStringLiteral("Calculation summary");
    // Keep the source-language string here; LanguageManager tracks this widget's original text so
    // toggling English ↔ Arabic remains reversible for dynamic result updates.
    return title;
}
}

// ---------------------------------------------------------------------------- RegisterView
RegisterView::RegisterView(QWidget* parent) : QFrame(parent) {
    Ui::RegisterView form;
    form.setupUi(this);
    m_layout = form.registerLayout;
}

void RegisterView::updateTextAlignment() {
    const bool rtl = LanguageManager::instance().isArabic();
    const auto labels = findChildren<QLabel*>();
    for (QLabel* label : labels) {
        const QString name = label->objectName();
        if (name == "regTitle") {
            const bool mode = label->property("regMode").toBool();
            const Qt::Alignment horizontal = mode
                ? (rtl ? Qt::AlignLeft : Qt::AlignRight)
                : (rtl ? Qt::AlignRight : Qt::AlignLeft);
            if (mode) label->setLayoutDirection(Qt::LeftToRight);
            label->setAlignment(horizontal | Qt::AlignVCenter);
        } else if (name == "regLine" || name == "regCaption" || name == "regBig" || name == "lcdBig" || name == "regFoot") {
            // Keep formulas, symbols, units and numeric values in a stable LTR run in Arabic UI.
            label->setLayoutDirection(Qt::LeftToRight);
            label->setAlignment((rtl ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignTop);
        }
    }
}

void RegisterView::changeEvent(QEvent* event) {
    QFrame::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange)
        updateTextAlignment();
}

void RegisterView::setRegister(const LcdRegister& reg) {
    clearLayout(m_layout);
    const auto prepareText = [](QLabel* label) {
        label->setWordWrap(true);
        label->setMinimumWidth(0);
        label->setTextFormat(Qt::PlainText);
    };

    auto* top = new QHBoxLayout;
    auto* title = new QLabel(displayResultTitle(reg.title));
    title->setObjectName("regTitle");
    title->setWordWrap(true);
    title->setMinimumWidth(0);
    title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Minimum);
    auto* mode = new QLabel(reg.isError ? "● ERR" : QString(" ") + QString::fromStdString(reg.mode));
    mode->setObjectName("regTitle");
    mode->setProperty("regMode", true);
    mode->setWordWrap(true);
    mode->setMinimumWidth(0);
    mode->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    top->addWidget(title, 1);
    top->addWidget(mode);
    m_layout->addLayout(top);

    auto addDivider = [this](int width = -1) {
        auto* d = new QFrame;
        d->setObjectName("regDivider");
        if (width > 0) d->setMaximumWidth(width);
        m_layout->addWidget(d);
    };
    addDivider();
    m_layout->addSpacing(1);

    for (const std::string& line : reg.lines) {
        auto* l = new QLabel(QString::fromStdString(line));
        l->setObjectName("regLine");
        prepareText(l);
        m_layout->addWidget(l);
    }
    if (!reg.resultValue.empty()) {
        m_layout->addSpacing(2);
        auto* cap = new QLabel(QString::fromStdString(reg.resultLabel));
        cap->setObjectName("regCaption");
        prepareText(cap);
        auto* big = new QLabel(QString::fromStdString(reg.resultValue));
        big->setObjectName("lcdBig");
        big->setWordWrap(true);
        big->setMinimumWidth(0);
        big->setTextFormat(Qt::PlainText);
        m_layout->addWidget(cap);
        m_layout->addWidget(big);
    }
    m_layout->addSpacing(2);
    addDivider(125);
    auto* foot = new QLabel(QString::fromStdString(reg.footer));
    foot->setObjectName("regFoot");
    prepareText(foot);
    m_layout->addWidget(foot);
    updateTextAlignment();
}

// ---------------------------------------------------------------------------- ResultCard
ResultCard::ResultCard(QWidget* parent) : QFrame(parent) {
    Ui::ResultCard form;
    form.setupUi(this);
    m_title = form.resultCardTitle;
    m_caption = form.resultCardCaption;
    m_value = form.lcdBig; // retained semantic selector used by interaction tests
    m_details = form.resultCardDetails;
}

void ResultCard::setRegister(const LcdRegister& result) {
    m_title->setText(displayResultTitle(result.title));
    m_caption->setText(QString::fromStdString(result.resultLabel));
    m_value->setText(QString::fromStdString(result.resultValue));
    QStringList details;
    for (const auto& line : result.lines) details << QString::fromStdString(line);
    for (const auto& line : result.extraLines) details << QString::fromStdString(line);
    m_details->setText(details.join(QLatin1Char('\n')));
    m_caption->setVisible(!result.resultLabel.empty());
    m_details->setVisible(!details.isEmpty());
}

// ---------------------------------------------------------------------------- NoticeCard
NoticeCard::NoticeCard(QWidget* parent) : QFrame(parent) {
    Ui::NoticeCard form;
    form.setupUi(this);
    m_title = form.noticeTitle;
    m_body = form.noticeBody;
}

void NoticeCard::setNotice(const QString& title, const QString& body, bool error) {
    m_title->setText(LanguageManager::instance().text(title));
    m_body->setText(LanguageManager::instance().text(body));
    setProperty("error", error);
    style()->unpolish(this);
    style()->polish(this);
}
void NoticeCard::setNotice(const Notice& n) {
    setNotice(QString::fromStdString(n.title), QString::fromStdString(n.body), n.error);
}

// ---------------------------------------------------------------------------- CalculatorKeypad
CalculatorKeypad::CalculatorKeypad(QWidget* parent) : QWidget(parent) {
    Ui::CalculatorKeypad form;
    form.setupUi(this);

    struct Key {
        const char* label;
        const char* insert;
        const char* kind;
    }; // insert: text to type, or "" for special keys
    const Key rows[7][8] = {
        {{"sin", "sin(", "calcScientific"}, {"cos", "cos(", "calcScientific"}, {"tan", "tan(", "calcScientific"},
         {"sec", "sec(", "calcScientific"}, {"csc", "csc(", "calcScientific"}, {"cot", "cot(", "calcScientific"},
         {"asin", "asin(", "calcScientific"}, {"acos", "acos(", "calcScientific"}},
        {{"atan", "atan(", "calcScientific"}, {"√", "sqrt(", "calcScientific"}, {"abs", "abs(", "calcScientific"},
         {"ln", "ln(", "calcScientific"}, {"log", "log(", "calcScientific"}, {"sinh", "sinh(", "calcScientific"},
         {"cosh", "cosh(", "calcScientific"}, {"tanh", "tanh(", "calcScientific"}},
        {{"exp", "exp(", "calcScientific"}, {"floor", "floor(", "calcScientific"}, {"ceil", "ceil(", "calcScientific"},
         {"round", "round(", "calcScientific"}, {"sgn", "sgn(", "calcScientific"}, {"x²", "^2", "calcScientific"},
         {"x³", "^3", "calcScientific"}, {"1/x", "1/(", "calcScientific"}},
        {{"7", "7", "calcNumber"}, {"8", "8", "calcNumber"}, {"9", "9", "calcNumber"}, {"÷", "÷", "calcOperator"},
         {"(", "(", "calcOperator"}, {")", ")", "calcOperator"}, {"DEL", "", "calcAction"}, {"AC", "", "calcAction"}},
        {{"4", "4", "calcNumber"}, {"5", "5", "calcNumber"}, {"6", "6", "calcNumber"}, {"×", "×", "calcOperator"},
         {"π", "π", "calcScientific"}, {"e", "e", "calcScientific"}, {"Ans", "Ans", "calcScientific"},
         {"nm", "nm", "calcScientific"}},
        {{"1", "1", "calcNumber"}, {"2", "2", "calcNumber"}, {"3", "3", "calcNumber"}, {"−", "-", "calcOperator"},
         {"cm", "cm", "calcScientific"}, {"mm", "mm", "calcScientific"}, {"µm", "um", "calcScientific"},
         {"^", "^", "calcOperator"}},
        {{"0", "0", "calcNumber"}, {"", "", ""}, {".", ".", "calcNumber"}, {"", "", ""},
         {"+", "+", "calcOperator"}, {"", "", ""}, {"=", "", "calcEquals"}, {"", "", ""}}};

    for (int r = 0; r < 7; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Key& k = rows[r][c];
            if (!*k.label) continue;
            const QString uiName = QStringLiteral("key%1%2").arg(r).arg(c);
            auto* b = findChild<QPushButton*>(uiName);
            Q_ASSERT_X(b, "CalculatorKeypad", qPrintable("Missing Designer key: " + uiName));
            if (!b) continue;
            b->setObjectName(k.kind);
            b->setMaximumHeight(36);
            b->setCursor(Qt::PointingHandCursor);
            const QString label = k.label, text = k.insert;
            connect(b, &QPushButton::clicked, this, [this, label, text] {
                if (label == "DEL") emit backspace();
                else if (label == "AC") emit clearAll();
                else if (label == "=") emit equals();
                else emit insertText(text);
            });
        }
    }
}

void showHelpDialog(QWidget* parent, const QString& title, const QString& html) {
    auto* dlg = new QDialog(parent);
    dlg->setWindowTitle(title);
    dlg->setMinimumSize(520, 400);
    auto* lay = new QVBoxLayout(dlg);
    auto* browser = new QTextBrowser;
    browser->setOpenExternalLinks(false);
    browser->setHtml(html);
    lay->addWidget(browser, 1);
    auto* btn = new QPushButton("Got it");
    btn->setObjectName("primary");
    btn->setDefault(true);
    QObject::connect(btn, &QPushButton::clicked, dlg, &QDialog::accept);
    lay->addWidget(btn, 0, Qt::AlignRight);
    dlg->exec();
    dlg->deleteLater();
}

} // namespace phy104
