#include "Components.h"
#include "ExpressionEvaluator.h"
#include "Icons.h"
#include "LanguageManager.h"
#include "ThemeManager.h"

#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QLocale>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>
#include <QDebug>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QVBoxLayout>
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
    l->setSpacing(3);
    auto* cap = new QLabel(label);
    cap->setObjectName("fieldLabel");
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
    setObjectName("panel");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* header = new QFrame;
    header->setObjectName("panelHeader");
    header->setFixedHeight(36);
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(12, 0, 12, 0);
    m_title = new QLabel(title);
    m_title->setObjectName("panelTitle");
    m_title->setWordWrap(true);
    m_title->setMinimumWidth(0);
    m_meta = new QLabel(meta.toUpper());
    m_meta->setObjectName("panelMeta");
    m_meta->setWordWrap(true);
    m_meta->setMinimumWidth(0);
    hl->addWidget(m_title, 1);
    hl->addStretch();
    hl->addWidget(m_meta);
    root->addWidget(header);

    auto* bodyHolder = new QWidget;
    m_body = new QVBoxLayout(bodyHolder);
    m_body->setContentsMargins(12, 12, 12, 12);
    m_body->setSpacing(12);
    root->addWidget(bodyHolder, 1);
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

void Panel::setMeta(const QString& meta) { m_meta->setText(meta.toUpper()); }
void Panel::setTitle(const QString& title) { m_title->setText(title); m_title->setWordWrap(true); m_title->setMinimumWidth(0); }

// ---------------------------------------------------------------------------- UnitField
UnitField::UnitField(const QString& unit, QWidget* parent) : QFrame(parent) {
    setObjectName("field");
    setFixedHeight(32);
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(10, 0, 4, 0);
    l->setSpacing(6);
    m_edit = new QLineEdit;
    m_edit->setFrame(false);
    m_edit->setLayoutDirection(Qt::LeftToRight);       // keep numeric expressions and scientific symbols stable in Arabic UI
    m_edit->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_edit->installEventFilter(this);
    installEventFilter(this);
    m_unit = new QLabel(unit);
    m_unit->setObjectName("unit");
    m_unit->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    // clickable arrows ▲▼
    m_arrows = new QWidget;
    auto* al = new QVBoxLayout(m_arrows);
    al->setContentsMargins(0, 2, 0, 2);
    al->setSpacing(0);
    auto makeArrow = [&](const char* name, const char* icon, int dir) {
        auto* b = new QToolButton;
        b->setObjectName(name);
        b->setFixedSize(18, 13);
        b->setAutoRaise(true);
        b->setAutoRepeat(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        b->setIconSize(QSize(12, 12));
        b->setProperty("iconName", icon);
        connect(b, &QToolButton::clicked, this, [this, dir] { step(dir); });
        al->addWidget(b);
        return b;
    };
    makeArrow("spinUp", "chevron-up", +1);
    makeArrow("spinDown", "chevron-down", -1);
    m_arrows->hide();

    l->addWidget(m_edit, 1);
    l->addWidget(m_unit);
    l->addWidget(m_arrows);

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
    auto* l = new QHBoxLayout(this);
    l->setContentsMargins(0, 0, 0, 0);
    l->setSpacing(8);
    int id = 0;
    for (const QString& label : labels) {
        auto* b = new QPushButton(QString(label).replace("&", "&&"));
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

// ---------------------------------------------------------------------------- RegisterView
RegisterView::RegisterView(QWidget* parent) : QFrame(parent) {
    setObjectName("register");
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(10, 8, 10, 8);
    m_layout->setSpacing(4);
}

void RegisterView::updateTextAlignment() {
    const bool rtl = LanguageManager::instance().isArabic();
    const auto labels = findChildren<QLabel*>();
    for (QLabel* label : labels) {
        const Q
