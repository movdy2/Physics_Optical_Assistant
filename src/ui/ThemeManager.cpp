#include "ThemeManager.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>
#include <QLineEdit>
#include <QPalette>
#include <QSettings>
#include <QStringList>
#include <QWidget>
#include <QStyleFactory>

namespace phy104 {

// ============================================================================
// Theme Definitions
// ============================================================================

Theme Theme::warmDark() {
    Theme t;
    t.dark = true;

    // surfaces — deep slate/charcoal, no pure black
    t.bg = "#12161F";
    t.sidebar = "#1A1D24";
    t.titleBar = "#10141B";
    t.card = "#1A1D24";
    t.cardHover = "#232A36";
    t.input = "#12161F";
    t.canvas = "#10141B";
    t.grid = "#1E2736";
    t.border = "#2E3B4E";
    t.surface = "#1A1D24";
    t.surfaceHover = "#232A36";

    // text — soft off-white, not stark white
    t.text = "#F0F4F8";
    t.muted = "#A3B8CC";
    t.dim = "#6B7A8C";

    // accent — calming cyan/teal + soft amber
    t.accentCyan = "#38BDF8";
    t.accentCyanHover = "#2FA3D8";
    t.accentCyanPressed = "#0E7490";
    t.accentCyanTint = QColor(56, 189, 248, 36);
    t.accentAmber = "#FBBF24";
    t.accentAmberHover = "#F59E0B";
    t.accentAmberPressed = "#B45309";
    t.accentAmberTint = QColor(251, 191, 36, 36);
    t.accentTeal = "#22D3EE";
    t.accentTealHover = "#0891B2";

    // semantic
    t.lime = "#4ADE80";
    t.error = "#F87171";
    t.errorBg = "rgba(248, 113, 113, 8)";

    // legacy names
    t.yellow = t.accentAmber;
    t.green = "#22C55E";
    t.red = "#EF4444";
    t.orange = "#F97316";
    t.blue = "#3B82F6";
    t.aqua = "#22D3EE";

    // Casio-style LCD colours
    t.lcdBg = "#9bb094";
    t.lcdText = "#0b1c08";
    t.lcdFrame = "#171e15";
    t.lcdDivider = "#708369";

    return t;
}

Theme Theme::paperLight() {
    Theme t;
    t.dark = false;

    // surfaces — warm off-white/cream, prevents glare
    t.bg = "#F4F1EA";
    t.sidebar = "#EAE5D8";
    t.titleBar = "#EFE9DD";
    t.card = "#FFFFFF";
    t.cardHover = "#F0EBE0";
    t.input = "#FFFFFF";
    t.canvas = "#EFE9DD";
    t.grid = "#E2D8C8";
    t.border = "#D8D0C2";
    t.surface = "#FFFFFF";
    t.surfaceHover = "#F0EBE0";

    // text — deep warm charcoal, not pure black
    t.text = "#1A1D24";
    t.muted = "#6B7280";
    t.dim = "#9CA3AF";

    // accent — slightly deeper cyan for light mode
    t.accentCyan = "#0E7490";
    t.accentCyanHover = "#0B6E84";
    t.accentCyanPressed = "#085F75";
    t.accentCyanTint = QColor(14, 116, 144, 34);
    t.accentAmber = "#B45309";
    t.accentAmberHover = "#D97706";
    t.accentAmberPressed = "#92400E";
    t.accentAmberTint = QColor(181, 83, 9, 34);
    t.accentTeal = "#0891B2";
    t.accentTealHover = "#0C85A8";

    // semantic
    t.lime = "#15803D";
    t.error = "#DC2626";
    t.errorBg = "rgba(220, 38, 38, 8)";

    // legacy names
    t.yellow = t.accentAmber;
    t.green = "#15803D";
    t.red = "#DC2626";
    t.orange = "#EA580C";
    t.blue = "#2563EB";
    t.aqua = "#0891B2";

    // Casio-style LCD colours
    t.lcdBg = "#9bb094";
    t.lcdText = "#0b1c08";
    t.lcdFrame = "#171e15";
    t.lcdDivider = "#708369";

    return t;
}

// Legacy themes kept for backward compatibility (delegates to new palettes)
Theme Theme::figmaDark() { return warmDark(); }
Theme Theme::figmaLight() { return paperLight(); }

// ============================================================================
// ThemeManager
// ============================================================================

ThemeManager& ThemeManager::instance() {
    static ThemeManager manager;
    return manager;
}

void ThemeManager::loadFonts() {
    for (const char* f : {\":/fonts/Inter-Regular.otf\", \":/fonts/Inter-Medium.otf\", \":/fonts/Inter-SemiBold.otf\",
                          \":/fonts/NotoSansArabic.ttf\",
                          \":/fonts/JetBrainsMono-Regular.ttf\", \":/fonts/JetBrainsMono-Bold.ttf\",
                          \":/fonts/DSEG14Classic-Bold.ttf\", \":/fonts/DSEG7Classic-Bold.ttf\"})
        QFontDatabase::addApplicationFont(f);
}

namespace {
QSettings* settingsScope() {
    static QSettings settings("PHY104", "PHY104OpticalAssistant");
    return &settings;
}
}

bool ThemeManager::storedDarkTheme() { return settingsScope()->value("ui/dark", true).toBool(); }

void ThemeManager::setLayoutDirectionForLanguage(bool isArabic) {
    QApplication::setLayoutDirection(isArabic ? Qt::RightToLeft : Qt::LeftToRight);
}

void ThemeManager::apply(QApplication& app, bool dark) {
    settingsScope()->setValue("ui/dark", dark);
    m_theme = dark ? Theme::warmDark() : Theme::paperLight();
    const Theme& t = m_theme;

    app.setStyle(QStyleFactory::create("Fusion"));
    QFont font(uiFont());
    font.setFamilies(uiFontFamilies());
    font.setPixelSize(13);
    app.setFont(font);

    QPalette p;
    p.setColor(QPalette::Window, t.bg);
    p.setColor(QPalette::WindowText, t.text);
    p.setColor(QPalette::Base, t.input);
    p.setColor(QPalette::AlternateBase, t.card);
    p.setColor(QPalette::Text, t.text);
    p.setColor(QPalette::Button, t.card);
    p.setColor(QPalette::ButtonText, t.text);
    p.setColor(QPalette::Highlight, t.accentCyan);
    p.setColor(QPalette::HighlightedText, t.bg);
    p.setColor(QPalette::ToolTipBase, t.card);
    p.setColor(QPalette::ToolTipText, t.text);
    p.setColor(QPalette::PlaceholderText, t.dim);
    p.setColor(QPalette::Link, t.accentCyan);
    p.setColor(QPalette::Disabled, QPalette::Text, t.dim);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, t.dim);
    app.setPalette(p);

    app.setStyleSheet(styleSheet(t));
    emit themeChanged();
}

QString ThemeManager::styleSheet(const Theme& t) {
    // Token-based QSS template. Tokens are replaced below with actual QColor values.
    QString css = R"CSS(
QMainWindow, QWidget#pageArea, QWidget { background: @bg@; color: @text@;
    font-family: 'Inter', 'Noto Sans Arabic', 'DejaVu Sans', 'Segoe UI', 'Arial';
    font-size: 13px; selection-background-color: @accentCyan@; selection-color: @bg@; }

QToolTip { background: @surface@; color: @text@; border: 1px solid @border@;
    border-radius: 6px; padding: 6px 10px; font-size: 12px; }

QFrame#sidebar { background: @surface@; border-right: 1px solid @border@; }
QPushButton#navItem { text-align: left; padding: 10px 16px; border: none;
    border-left: 3px solid transparent; border-radius: 0; background: transparent;
    color: @muted@; font-size: 13px; font-weight: 500; min-height: 44px; }
QPushButton#navItem:hover { background: @surfaceHover@; color: @text@;
    border-left: 3px solid @accentCyan@; }
QPushButton#navItem:checked { background: rgba(56,189,248,12);
    border-left: 3px solid @accentCyan@; color: @accentCyan@; font-weight: 600; }
QLabel#navSection { font-family: 'JetBrains Mono'; font-size: 9px; color: @dim@;
    padding: 10px 14px 2px 14px; }

QLabel#pageTitle { font-size: 26px; font-weight: 700; color: @text@; padding-bottom: 4px; }
QLabel#pageSubtitle { font-size: 13px; color: @muted@; padding-bottom: 16px; }
QLabel#sectionTitle { font-size: 13px; font-weight: 700; color: @accentAmber@;
    text-transform: uppercase; letter-spacing: 0.5px; padding-top: 16px; padding-bottom: 8px; }
QLabel#muted { color: @muted@; }
QLabel#caption { font-family: 'JetBrains Mono'; font-size: 9px; color: @dim@; }
QLabel#colourSwatch { border: 1px solid @border@; border-radius: 4px; min-height: 30px; }

QFrame#panel { background: @surface@; border: 1px solid @border@;
    border-radius: 10px; padding: 16px; }
QFrame#panel[hover="true"] { border-color: @accentCyan@; background: @surfaceHover@; }
QLabel#panelTitle { font-size: 14px; font-weight: 700; color: @text@; }
QLabel#panelMeta { font-family: 'JetBrains Mono'; font-size: 11px; color: @dim@; }

QLabel#fieldLabel { font-size: 12px; font-weight: 700; color: @muted@;
    padding-bottom: 4px; }
QLabel#hint { font-size: 11px; color: @dim@; }

QLineEdit { background: @surface@; color: @text@; border: 1px solid @border@;
    border-radius: 8px; padding: 10px 14px; font-family: 'JetBrains Mono';
    font-size: 13px; min-height: 42px; }
QLineEdit:focus { border: 2px solid @accentCyan@; background: @surfaceHover@; }
QLineEdit[error="true"] { border: 2px solid @error@; }
QLineEdit[error="true"]:focus { border: 2px solid @error@; }

QLabel#unit { font-family: 'JetBrains Mono'; font-size: 12px; color: @dim@;
    padding: 0 8px; background: transparent; border: none; }
QLabel#fieldError { color: @error@; font-size: 11px; font-weight: 600;
    padding-top: 4px; }

QComboBox { background: @surface@; color: @text@; border: 1px solid @border@;
    border-radius: 8px; padding: 10px 14px; min-height: 42px; min-width: 150px;
    font-family: 'JetBrains Mono'; font-size: 13px;
    selection-background-color: @accentCyan@; selection-color: @bg@; }
QComboBox:focus { border: 2px solid @accentCyan@; background: @surfaceHover@; }
QComboBox::drop-down { border: none; width: 30px; }
QComboBox::down-arrow { image: url(:/icons/chevron-down.svg); width: 16px; height: 16px; }
QComboBox QAbstractItemView { background: @surface@; color: @text@;
    border: 1px solid @border@; selection-background-color: @accentCyan@;
    selection-color: @bg@; outline: 0; padding: 6px; min-width: 150px; }
QComboBox QAbstractItemView::item { padding: 10px 14px; border-radius: 6px; margin: 2px; }
QComboBox QAbstractItemView::item:selected { background: @accentCyan@; color: @bg@; }

QPushButton { background: @surface@; color: @text@; border: 1px solid @border@;
    border-radius: 8px; padding: 10px 20px; font-size: 13px; font-weight: 600;
    min-height: 44px; }
QPushButton:hover { background: @surfaceHover@; border-color: @accentCyan@; }
QPushButton:pressed { background: @border@; }
QPushButton:disabled { color: @dim@; background: @bg@; border-color: @bg@; }

QPushButton#primary { background: @accentCyan@; color: @bg@; border: none;
    font-weight: 700; font-size: 14px; min-height: 48px; padding: 0 24px; }
QPushButton#primary:hover { background: #2FA3D8; }
QPushButton#primary:pressed { background: #0E7490; }

QPushButton#secondary { background: @surface@; color: @text@;
    border: 1px solid @border@; font-weight: 600; font-size: 13px;
    min-height: 44px; padding: 0 20px; }
QPushButton#secondary:hover { border-color: @accentCyan@; background: @surfaceHover@; }

QPushButton#outlineAmber, QPushButton#sendToNotes {
    background: transparent; color: @accentAmber@; border: 1px solid @accentAmber@;
    border-radius: 8px; font-weight: 600; font-size: 13px; min-height: 44px;
    padding: 0 20px; }
QPushButton#outlineAmber:hover, QPushButton#sendToNotes:hover {
    background: rgba(251,191,36,12); }

QPushButton#presetBtn { background: @surface@; color: @text@;
    border: 1px solid @border@; border-radius: 8px; padding: 8px 14px;
    font-size: 12px; font-weight: 600; min-width: 75px; min-height: 34px;
    text-align: center; }
QPushButton#presetBtn:hover { border-color: @accentAmber@; background: @surfaceHover@; }
QPushButton#presetBtn:pressed { background: @bg@; }

QPushButton#modeTab { background: @surfaceHover@; color: @text@;
    border: 1px solid @border@; border-radius: 8px; min-height: 34px;
    font-size: 13px; font-weight: 500; padding: 0 16px; }
QPushButton#modeTab:hover { border-color: @accentCyan@; color: @accentCyan@; }
QPushButton#modeTab:checked { background: @accentCyan@; color: @bg@;
    border: none; font-weight: 600; }

QCheckBox { color: @text@; spacing: 10px; font-size: 13px; }
QCheckBox::indicator { width: 20px; height: 20px; border: 2px solid @border@;
    border-radius: 5px; background: @surface@; }
QCheckBox::indicator:checked { background: @accentCyan@; border-color: @accentCyan@;
    image: url(:/icons/check.svg); }
QCheckBox::indicator:hover { border-color: @accentCyan@; }

QSlider::groove:horizontal { height: 8px; background: @border@; border-radius: 4px; }
QSlider::sub-page:horizontal { background: @accentCyan@; border-radius: 4px; }
QSlider::handle:horizontal { background: @accentCyan@; width: 20px; height: 20px;
    margin: -7px 0; border-radius: 10px; border: 2px solid @bg@; }
QSlider::handle:horizontal:hover { width: 22px; height: 22px; margin: -8px 0;
    border-radius: 11px; }

QFrame#register { background: @surface@; border: 1px solid @border@;
    border-radius: 10px; }
QLabel#regBig { font-family: 'JetBrains Mono'; font-size: 20px;
    font-weight: 700; color: @accentAmber@; }
QLabel#regLine { font-family: 'JetBrains Mono'; font-size: 12px;
    color: @muted@; line-height: 1.6; }
QLabel#regCaption { font-family: 'JetBrains Mono'; font-size: 9px; color: @dim@; }
QLabel#regFoot { font-family: 'JetBrains Mono'; font-size: 9px; color: @dim@; }

QFrame#notice { background: @surface@; border: 1px solid @border@;
    border-radius: 8px; padding: 12px 16px; }
QFrame#notice[error="true"] { background: rgba(248,113,113,8);
    border: 1px solid @error@; }
QLabel#noticeTitle { font-size: 13px; font-weight: 700; color: @accentAmber@; }
QFrame#notice[error="true"] QLabel#noticeTitle { color: @error@; }
QLabel#noticeBody { font-size: 12px; color: @muted@; line-height: 1.5; }

QScrollArea { background: transparent; border: none; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: @dim@; border-radius: 5px;
    min-height: 30px; }
QScrollBar::handle:vertical:hover { background: @muted@; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: @dim@; border-radius: 5px;
    min-width: 28px; }
QScrollBar::handle:horizontal:hover { background: @muted@; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

QStatusBar { background: @surface@; color: @dim@;
    border-top: 1px solid @border@; font-size: 12px; }
)CSS";

    const std::pair<const char*, QColor> tokens[] = {
        {"@bg@", t.bg}, {"@surface@", t.surface}, {"@surfaceHover@", t.surfaceHover},
        {"@border@", t.border}, {"@text@", t.text}, {"@muted@", t.muted},
        {"@dim@", t.dim},
        {"@accentCyan@", t.accentCyan}, {"@accentCyanHover@", t.accentCyanHover},
        {"@accentCyanPressed@", t.accentCyanPressed},
        {"@accentCyanTint@", t.accentCyanTint},
        {"@accentAmber@", t.accentAmber}, {"@accentAmberHover@", t.accentAmberHover},
        {"@accentAmberPressed@", t.accentAmberPressed},
        {"@accentAmberTint@", t.accentAmberTint},
        {"@accentTeal@", t.accentTeal}, {"@accentTealHover@", t.accentTealHover},
        {"@lime@", t.lime}, {"@error@", t.error}, {"@errorBg@", t.errorBg},
        {"@canvas@", t.canvas},
        {"@calcDim@", t.dark ? QColor(Qt::white) : t.text}};
    for (const auto& [token, color] : tokens) css.replace(token, color.name());

    // RGBA tints need alpha channel preserved
    css.replace("@accentCyanTint@",
                QString("rgba(%1,%2,%3,%4)")
                    .arg(t.accentCyanTint.red())
                    .arg(t.accentCyanTint.green())
                    .arg(t.accentCyanTint.blue())
                    .arg(t.accentCyanTint.alpha()));
    css.replace("@accentAmberTint@",
                QString("rgba(%1,%2,%3,%4)")
                    .arg(t.accentAmberTint.red())
                    .arg(t.accentAmberTint.green())
                    .arg(t.accentAmberTint.blue())
                    .arg(t.accentAmberTint.alpha()));

    // Icon path selection based on theme
    css.replace("@chevron@", t.dark ? ":/icons/chevron-down-dark.svg"
                                    : ":/icons/chevron-down-light.svg");

    return css;
}

} // namespace phy104
