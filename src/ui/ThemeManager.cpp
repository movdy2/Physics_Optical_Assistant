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
    // Gruvbox dark palette
    t.bg = "#282828";           // dark0 — main background
    t.sidebar = "#32302f";      // dark1 — sidebar
    t.titleBar = "#1d2021";     // dark0_hard — title bar
    t.card = "#32302f";         // dark1 — cards
    t.cardHover = "#3a3735";    // dark2 — card hover
    t.input = "#282828";        // dark0 — input fields
    t.canvas = "#1d2021";       // dark0_hard — canvas
    t.grid = "#32302f";         // dark1 — grid lines
    t.border = "#3a3735";       // dark2 — borders
    t.surface = "#32302f";      // dark1 — surfaces
    t.surfaceHover = "#3a3735"; // dark2 — surface hover
    t.text = "#fbf1c7";         // light0 — main text
    t.muted = "#f2e5bc";        // light1 — muted text
    t.dim = "#ebdbb2";          // light2 — dim text
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
    t.lime = "#4ADE80";
    t.error = "#F87171";
    t.errorBg = "rgba(248, 113, 113, 8)";
    t.yellow = t.accentAmber;
    t.green = "#22C55E";
    t.red = "#EF4444";
    t.orange = "#F97316";
    t.blue = "#3B82F6";
    t.aqua = "#22D3EE";
    t.lcdBg = "#9bb094";
    t.lcdText = "#0b1c08";
    t.lcdFrame = "#171e15";
    t.lcdDivider = "#708369";
    return t;
}

Theme Theme::paperLight() {
    Theme t;
    t.dark = false;
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
    t.text = "#1A1D24";
    t.muted = "#6B7280";
    t.dim = "#9CA3AF";
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
    t.lime = "#15803D";
    t.error = "#DC2626";
    t.errorBg = "rgba(220, 38, 38, 8)";
    t.yellow = t.accentAmber;
    t.green = "#15803D";
    t.red = "#DC2626";
    t.orange = "#EA580C";
    t.blue = "#2563EB";
    t.aqua = "#0891B2";
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
