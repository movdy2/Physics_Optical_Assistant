#include "ThemeManager.h"

#include <QApplication>
#include <QFile>
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
// Theme Definitions (Gruvbox Dark + Gruvbox Light)
// ============================================================================

Theme Theme::gruvboxDark() {
    Theme t;
    t.dark = true;
    // Classic Gruvbox dark: warm charcoal surfaces, parchment text, yellow + aqua accents.
    t.bg = "#282828";
    t.sidebar = "#3c3836";
    t.titleBar = "#1d2021";
    t.card = "#3c3836";
    t.cardHover = "#504945";
    t.input = "#282828";
    t.canvas = "#1d2021";
    t.grid = "#3c3836";
    t.border = "#504945";
    t.surface = "#3c3836";
    t.surfaceHover = "#504945";
    t.text = "#ebdbb2";
    t.muted = "#d5c4a1";
    t.dim = "#bdae93";
    t.accentCyan = "#83a598";
    t.accentCyanHover = "#8ec07c";
    t.accentCyanPressed = "#689d6a";
    t.accentCyanTint = QColor(131, 165, 152, 36);
    t.accentAmber = "#d79921";
    t.accentAmberHover = "#fabd2f";
    t.accentAmberPressed = "#b57614";
    t.accentAmberTint = QColor(215, 153, 33, 36);
    t.accentTeal = "#8ec07c";
    t.accentTealHover = "#689d6a";
    t.lime = "#b8bb26";
    t.error = "#fb4934";
    t.errorBg = QColor(251, 73, 52, 20);
    t.yellow = t.accentAmber;
    t.green = "#b8bb26";
    t.red = "#fb4934";
    t.orange = "#fe8019";
    t.blue = "#83a598";
    t.aqua = "#8ec07c";
    t.amber = t.accentAmber;
    t.sage = t.accentTeal;
    return t;
}

Theme Theme::gruvboxLight() {
    Theme t;
    t.dark = false;
    // Gruvbox light keeps the same recognizable yellow/aqua semantics and clear contrast.
    t.bg = "#fbf1c7";
    t.sidebar = "#ebdbb2";
    t.titleBar = "#f2e5bc";
    t.card = "#f9f5d7";
    t.cardHover = "#ebdbb2";
    t.input = "#f9f5d7";
    t.canvas = "#f2e5bc";
    t.grid = "#ebdbb2";
    t.border = "#d5c4a1";
    t.surface = "#f9f5d7";
    t.surfaceHover = "#ebdbb2";
    t.text = "#282828";
    t.muted = "#504945";
    t.dim = "#7c6f64";
    t.accentCyan = "#427b58";
    t.accentCyanHover = "#689d6a";
    t.accentCyanPressed = "#3c6c4b";
    t.accentCyanTint = QColor(66, 123, 88, 34);
    t.accentAmber = "#b57614";
    t.accentAmberHover = "#d79921";
    t.accentAmberPressed = "#8f5b0a";
    t.accentAmberTint = QColor(181, 118, 20, 34);
    t.accentTeal = "#689d6a";
    t.accentTealHover = "#427b58";
    t.lime = "#79740e";
    t.error = "#9d0006";
    t.errorBg = QColor(157, 0, 6, 16);
    t.yellow = t.accentAmber;
    t.green = "#79740e";
    t.red = "#9d0006";
    t.orange = "#af3a03";
    t.blue = "#427b58";
    t.aqua = "#689d6a";
    t.amber = t.accentAmber;
    t.sage = t.accentTeal;
    return t;
}

// ============================================================================
// ThemeManager
// ============================================================================

ThemeManager& ThemeManager::instance() {
    static ThemeManager manager;
    return manager;
}

void ThemeManager::loadFonts() {
   for (const char* f : {":/fonts/Inter-Regular.otf", ":/fonts/Inter-Medium.otf", ":/fonts/Inter-SemiBold.otf",
                          ":/fonts/NotoSansArabic.ttf",
                          ":/fonts/JetBrainsMono-Regular.ttf", ":/fonts/JetBrainsMono-Bold.ttf"})
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

QString ThemeManager::loadQssFromResource() {
    QFile file(":/styles/phy104.qss");
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning("ThemeManager: could not open :/styles/phy104.qss");
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

void ThemeManager::apply(QApplication& app, bool dark) {
    settingsScope()->setValue("ui/dark", dark);
    m_theme = dark ? Theme::gruvboxDark() : Theme::gruvboxLight();
    const Theme& t = m_theme;

    app.setStyle(QStyleFactory::create("Fusion"));
    QFont font(uiFont());
    font.setFamilies(uiFontFamilies());
    font.setPixelSize(12);   // compact
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

    QString qss = loadQssFromResource();

    const std::pair<const char*, QColor> tokens[] = {
        {"@bg@", t.bg}, {"@surface@", t.surface}, {"@surfaceHover@", t.surfaceHover},
        {"@border@", t.border}, {"@text@", t.text}, {"@muted@", t.muted},
        {"@dim@", t.dim},
        {"@accentCyan@", t.accentCyan}, {"@accentCyanHover@", t.accentCyanHover},
        {"@accentCyanPressed@", t.accentCyanPressed},
        {"@accentAmber@", t.accentAmber}, {"@accentAmberHover@", t.accentAmberHover},
        {"@accentAmberPressed@", t.accentAmberPressed},
        {"@accentTeal@", t.accentTeal}, {"@accentTealHover@", t.accentTealHover},
        {"@lime@", t.lime}, {"@error@", t.error},
        {"@canvas@", t.canvas},
        {"@calcDim@", t.dark ? QColor(Qt::white) : t.text}};
    for (const auto& [token, color] : tokens) qss.replace(token, color.name());

    qss.replace("@errorBg@",
                QString("rgba(%1,%2,%3,%4)")
                    .arg(t.errorBg.red())
                    .arg(t.errorBg.green())
                    .arg(t.errorBg.blue())
                    .arg(t.errorBg.alpha()));
    qss.replace("@accentCyanTint@",
                QString("rgba(%1,%2,%3,%4)")
                    .arg(t.accentCyanTint.red())
                    .arg(t.accentCyanTint.green())
                    .arg(t.accentCyanTint.blue())
                    .arg(t.accentCyanTint.alpha()));
    qss.replace("@accentAmberTint@",
                QString("rgba(%1,%2,%3,%4)")
                    .arg(t.accentAmberTint.red())
                    .arg(t.accentAmberTint.green())
                    .arg(t.accentAmberTint.blue())
                    .arg(t.accentAmberTint.alpha()));

    qss.replace("@chevron@", t.dark ? ":/icons/chevron-down-dark.svg"
                                    : ":/icons/chevron-down-light.svg");

    app.setStyleSheet(qss);
    emit themeChanged();
}

} // namespace phy104
