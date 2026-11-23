#pragma once
#include <QColor>
#include <QFont>
#include <QObject>
#include <QStringList>

class QApplication;
class QLineEdit;
class QWidget;

namespace phy104 {

/// Gruvbox theme colors shared by the application.
struct Theme {
    bool dark = true;

    // surfaces
    QColor bg, sidebar, titleBar, card, cardHover, input, canvas, grid, border;
    QColor surface, surfaceHover;

    // text
    QColor text, muted, dim;

    // accent (primary action, titles, active navigation)
    QColor accentPrimary, accentPrimaryHover, accentPrimaryPressed, accentPrimaryTint;
    QColor accentAmber, accentAmberHover, accentAmberPressed, accentAmberTint;
    QColor accentTeal, accentTealHover;

    // semantic
    QColor lime;        // valid / success
    QColor error, errorBg;

    // older names still used by the SVG diagrams, LaTeX, Plots and OpticsCanvases
    QColor yellow, green, red, orange, blue, aqua, amber, sage;

    static Theme gruvboxDark();
    static Theme gruvboxLight();
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

    /// Apply a theme to the application.
    /// @param dark true = Gruvbox dark, false = Gruvbox light
    void apply(QApplication& app, bool dark);

    /// Set the application layout direction based on the active language.
    /// Call this whenever the user toggles between English and Arabic.
    static void setLayoutDirectionForLanguage(bool isArabic);

    const Theme& theme() const { return m_theme; }
    bool isDark() const { return m_theme.dark; }

    /// The theme chosen in the last run (saved by apply(); read by main() before the UI is built).
    static bool storedDarkTheme();

signals:
    void themeChanged();

private:
    ThemeManager() = default;
    static QString styleSheet(const Theme& t);
    static QString loadQssFromResource();

    Theme m_theme = Theme::gruvboxDark();
};

} // namespace phy104
