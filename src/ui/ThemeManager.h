#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QStringList>

class QApplication;
class QLineEdit;
class QWidget;

namespace phy104 {

/// All colours of one theme. The dark theme is a warm slate/charcoal palette
/// designed for long study sessions; the light theme is a paper-like cream.
struct Theme {
    bool dark = true;

    // surfaces
    QColor bg, sidebar, titleBar, card, cardHover, input, canvas, grid, border;
    QColor surface, surfaceHover;

    // text
    QColor text, muted, dim;

    // accent (primary action, titles, active navigation)
    QColor accentCyan, accentCyanHover, accentCyanPressed, accentCyanTint;
    QColor accentAmber, accentAmberHover, accentAmberPressed, accentAmberTint;
    QColor accentTeal, accentTealHover;

    // semantic
    QColor lime;        // valid / success
    QColor error, errorBg;

    // older names still used by the SVG diagrams, LaTeX and formula cards
    QColor yellow, green, red, orange, blue, aqua;

    // Casio-style LCD colours
    QColor lcdBg, lcdText, lcdFrame, lcdDivider;

    static Theme warmDark();     // NEW: deep slate/charcoal for long sessions
    static Theme paperLight();   // NEW: warm cream paper-like light mode
    static Theme figmaDark();    // legacy, kept for backward compatibility
    static Theme figmaLight();   // legacy, kept for backward compatibility

    static Theme gruvboxDark() { return warmDark(); }
    static Theme gruvboxLight() { return paperLight(); }
};

/// Loads the bundled UI, Arabic fallback, and mono fonts, applies palette + style sheet,
/// and announces theme changes.
class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();

    static void loadFonts();                       // call once, before creating widgets
    static QString uiFont() { return "Inter"; }
    /// Shared sans-serif and glyph fallback order for English, Arabic and custom-painted UI text.
    static QStringList uiFontFamilies() { return {uiFont(), "Noto Sans Arabic", "DejaVu Sans", "Segoe UI", "Arial"}; }
    static QString monoFont() { return "JetBrains Mono"; }
    static QFont lcdFont() { return QFont("DSEG14 Classic"); }

    /// Apply a theme to the application.
    /// @param dark true = warm dark mode, false = paper-like light mode
    void apply(QApplication& app, bool dark);

    /// Set the application layout direction based on the active language.
    /// Call this whenever the user toggles between English and Arabic.
    static void setLayoutDirectionForLanguage(bool isArabic);

    const Theme& theme() const { return m_theme; }
    bool isDark() const { return m_theme.dark; }

    /// The theme chosen in the last run (saved by apply(); read by main() before the UI is built).
    static bool storedDarkTheme();

signals:
    void themeChanged():

private:
    ThemeManager() = default;
    static QString styleSheet(const Theme& t);

    Theme m_theme = Theme::warmDark();
};

} // namespace phy104
