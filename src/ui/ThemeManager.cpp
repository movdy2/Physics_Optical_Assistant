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
// Theme Definitions (Warm Dark + Paper Light)
// ============================================================================

Theme Theme::warmDark() {
    Theme t;
    t.dark = true;
    // Canonical Gruvbox dark palette
    t.bg = "#1d2021";        // bg_hard
    t.sidebar = "#282828";   // bg0
    t.titleBar = "#1d2021";  // bg_hard
    t.card = "#32302f";      // bg1
    t.cardHover = "#3c3836"; // bg3
    t.input = "#1d2021";     // bg_hard
    t.canvas = "#1d2021";    // bg_hard
    t.grid = "#32302f";      // bg1
    t.border = "#504945";    // bg2
    t.surface = "#282828";   // bg0
    t.surfaceHover = "#3c3836"; // bg3
    t.text = "#ebdbb2";      // fg
    t.muted = "#a89984";     // light4
    t.dim = "#665c54";       // gray3
    t.accentCyan = "#458588";   // blue normal
    t.accentCyanHover = "#83a598"; // blue bright
    t.accentCyanPressed = "#458588";
    t.accentCyanTint = QColor(69, 133, 136, 36);
    t.accentAmber = "#d79921";   // yellow normal
    t.accentAmberHover = "#fabd2f"; // yellow bright
    t.accentAmberPressed = "#d79921";
    t.accentAmberTint = QColor(215, 153, 33, 36);
    t.accentTeal = "#458588";
    t.accentTealHover = "#83a598";
    t.lime = "#98971a";    // green normal
    t.error = "#cc241d";   // red normal
    t.errorBg = "rgba(204, 36, 29, 8)";
    t.yellow = t.accentAmber;
    t.green = "#98971a";
    t.red = "#cc241d";
    t.orange = "#d65d0e";
    t.blue = "#458588";
    t.aqua = "#458588";
    t.sage = "#665c54";
    t.lcdBg = "#9bb094";
    t.lcdText = "#0b1c08";
    t.lcdFrame = "#171e15";
    t.lcdDivider = "#708369";
    return t;
}

Theme Theme::paperLight() {
    Theme t;
    t.dark = false;
    // Canonical Gruvbox light palette
    t.bg = "#fbf1c7";        // light bg0
    t.sidebar = "#ebdbb2";   // light bg1
    t.titleBar = "#fbf1c7";  // light bg0
    t.card = "#d5c4a1";      // light bg3
    t.cardHover = "#bdae93"; // light bg4
    t.input = "#fbf1c7";     // light bg0
    t.canvas = "#fbf1c7";    // light bg0
    t.grid = "#ebdbb2";      // light bg1
    t.border = "#bdae93";    // light bg4
    t.surface = "#ebdbb2";   // light bg1
    t.surfaceHover = "#d5c4a1"; // light bg3
    t.text = "#3c3836";      // dark0_hard
    t.muted = "#7c6f64";     // dark gray4
    t.dim = "#8c7a65";       // dark gray3
    t.accentCyan = "#458588";   // blue normal
    t.accentCyanHover = "#83a598"; // blue bright
    t.accentCyanPressed = "#458588";
    t.accentCyanTint = QColor(69, 133, 136, 34);
    t.accentAmber = "#d79921";   // yellow normal
    t.accentAmberHover = "#fabd2f"; // yellow bright
    t.accentAmberPressed = "#d79921";
    t.accentAmberTint = QColor(215, 153, 33, 34);
    t.accentTeal = "#458588";
    t.accentTealHover = "#83a598";
    t.lime = "#98971a";
    t.error = "#cc241d";
    t.errorBg = "rgba(204, 36, 29, 8)";
    t.yellow = t.accentAmber;
    t.green = "#98971a";
    t.red = "#cc241d";
    t.orange = "#d65d0e";
    t.blue = "#458588";
    t.aqua = "#458588";
    t.sage = "#665c54";
    t.lcdBg = "#9bb094";
    t.lcdText = "#0b1c08";
    t.lcdFrame = "#171e15";
    t.lcdDivider = "#708369";
    return t;
}

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
   for (const char* f : {":/fonts/Inter-Regular.otf", ":/fonts/Inter-Medium.otf", ":/fonts/Inter-SemiBold.otf",
                          ":/fonts/NotoSansArabic.ttf",
                          ":/fonts/JetBrainsMono-Regular.ttf", ":/fonts/JetBrainsMono-Bold.ttf",
                          ":/fonts/DSEG14Classic-Bold.ttf", ":/fonts/DSEG7Classic-Bold.ttf"})
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
    m_theme = dark ? Theme::warmDark() : Theme::paperLight();
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
        {"@bg@", t.bg}, {"@sidebar@", t.sidebar}, {"@surface@", t.surface}, {"@surfaceHover@", t.surfaceHover},
        {"@input@", t.input}, {"@card@", t.card}, {"@cardHover@", t.cardHover},
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
    for (const auto& [token, color] : tokens) qss.replace(token, color.name());

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
