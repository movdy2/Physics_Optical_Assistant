// UI-level checks that need Qt Widgets/Svg but no visible window (run with the offscreen platform):
//   * every LaTeX string in the JSON files is understood by our renderer
//   * every SVG diagram has only known colour tokens and renders with Qt in both themes
//   * Gruvbox dark/light palettes and application fonts are valid
#include "Icons.h"
#include "MarkdownHighlighter.h"
#include "JsonRepositories.h"
#include "LatexRenderer.h"
#include "SvgDiagramView.h"
#include "ThemeManager.h"

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

using namespace phy104;

static int g_failures = 0;
static void check(bool ok, const QString& what) {
    if (!ok) { ++g_failures; std::cerr << "FAIL: " << what.toStdString() << "\n"; }
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    const QString data = DATA_DIR;

    // ---- LaTeX ----
    int latexCount = 0;
    try {
        for (const auto& f : JsonFormulaRepository(data + "/formulas.json").loadAll()) {
            QString err;
            check(LatexRenderer::validate(QString::fromStdString(f.latex), &err), "LaTeX of formula " + QString::fromStdString(f.id) + ": " + err);
            ++latexCount;
        }
        for (const auto& v : JsonVariableRepository(data + "/variables.json").loadAll()) {
            QString err;
            check(!v.latex.empty(), "variable has LaTeX: " + QString::fromStdString(v.id));
            check(LatexRenderer::validate(QString::fromStdString(v.latex), &err), "LaTeX of variable " + QString::fromStdString(v.id) + ": " + err);
            ++latexCount;
        }
        for (const auto& note : JsonStudyNotesRepository(data + "/notes.json").loadAll()) {
            check(!note.id.empty() && !note.chapter.empty() && !note.type.empty() && !note.title.empty(),
                  "study note has all metadata: " + QString::fromStdString(note.id));
            for (const std::string& equation : note.equations) {
                QString err;
                check(LatexRenderer::validate(QString::fromStdString(equation), &err),
                      "LaTeX of study note " + QString::fromStdString(note.id) + ": " + err);
                ++latexCount;
            }
        }
    } catch (const std::exception& e) { check(false, e.what()); }
    check(latexCount > 0, "at least one LaTeX source was checked");
    for (const char* broken : {"\\frac{1}", "\\frac{", "\\sqrt", "x^", "x_", "\\left(", "{{", "}}", "\\frac{}{}", "^{}", "\\operatorname{min}()"}) {   // half-typed formulas must never crash
        const QPixmap p = LatexRenderer::render(broken, QFont(), Qt::white, 1.0);
        check(!p.isNull(), QString("renders without crashing: ") + broken);
    }
    check(!LatexRenderer::validate("\\notacommand{x}"), "unknown command is rejected");
    check(!LatexRenderer::validate("\\frac{1}{2"), "missing brace is rejected");
    check(LatexRenderer::validate("\\frac{1}{s}+\\frac{1}{s'}=\\frac{1}{f}"), "thin lens equation is accepted");
    QFont font; font.setPointSize(14);
    const QPixmap pix = LatexRenderer::render("\\sqrt{\\frac{a}{b}}", font, Qt::white, 2.0);
    check(!pix.isNull() && pix.width() > 10 && pix.height() > 10, "LaTeX renders to a pixmap");
    check(LatexRenderer::measure("\\frac{a}{b}", font).height() > LatexRenderer::measure("a", font).height(), "fraction is taller than a letter");

    // ---- theme colours: classic Gruvbox dark & light ----
    const Theme dark = Theme::gruvboxDark();
    check(dark.bg.isValid() && dark.card.isValid() && dark.text.isValid(), "dark theme core colours valid");
    check(dark.bg == QColor(0x28, 0x28, 0x28) && dark.sidebar == QColor(0x3c, 0x38, 0x36) &&
          dark.text == QColor(0xeb, 0xdb, 0xb2) && dark.accentAmber == QColor(0xd7, 0x99, 0x21) &&
          dark.accentPrimary == QColor(0xd7, 0x99, 0x21), "classic Gruvbox dark palette is restored");
    const Theme light = Theme::gruvboxLight();
    check(light.bg == QColor(0xfb, 0xf1, 0xc7) && light.sidebar == QColor(0xeb, 0xdb, 0xb2) &&
          light.text == QColor(0x28, 0x28, 0x28), "classic Gruvbox light palette is restored");
    check(dark.amber.isValid() && dark.sage.isValid() && dark.accentAmberHover.isValid() && dark.accentAmberPressed.isValid(), "Gruvbox amber/sage theme colors are valid");
    check(dark.border.isValid() && dark.error.isValid(), "border and error colours valid");
    const QStringList diagramFiles = QDir(":/diagrams").entryList({"*.svg"}, QDir::Files);
    check(diagramFiles.size() == 13, "all setup-diagram SVG resources are embedded");
    for (const QString& file : diagramFiles) {
        SvgDiagramView view;
        check(view.load(":/diagrams/" + file), "setup SVG renders: " + file);
    }
    ThemeManager::loadFonts();
    check(QFontDatabase::families().contains("Inter") && QFontDatabase::families().contains("JetBrains Mono"), "bundled fonts Inter + JetBrains Mono load");
    check(QFontDatabase::families().contains("Noto Sans Arabic"), "bundled Noto Sans Arabic fallback loads");
    check(QFile::exists(":/licenses/NotoSansArabic-OFL.txt"), "Noto Sans Arabic OFL license is embedded");
    for (const char* icon : {"layout-dashboard", "split", "focus", "triangle", "waves", "notebook-pen", "file-spreadsheet", "scan-line", "arrow-up-right", "sigma", "calculator", "ruler", "image", "download", "printer", "slit", "aperture"})
        check(!Icons::pixmap(icon, Qt::white, 18).isNull(), QString("icon exists: ") + icon);

    // ---- readable unicode maths -> LaTeX (notes preview, formula reference) ----
    {
        auto conv = [](const char* s) { return LatexRenderer::fromPlainMath(QString::fromUtf8(s)); };
        const QString snell = conv("n₁ sin θ₁ = n₂ sin θ₂");
        check(snell.contains("\\sin") && snell.contains("\\theta") && snell.contains("_{1}") && snell.contains("_{2}") && !snell.contains(QString::fromUtf8("θ")), "Snell: functions, Greek, subscripts");
        check(conv("1/f = 1/dₒ + 1/dᵢ").contains("\\frac{1}{f}") && conv("1/f = 1/dₒ + 1/dᵢ").contains("\\frac{1}{d_{o}}"), "fractions");
        check(conv("sin θc = n₂ / n₁").contains("\\frac{n_{2}}{n_{1}}"), "spaced fraction");
        check(conv("I = I₀ cos²θ").contains("^{2}") && conv("I = I₀ cos²θ").contains("I_{0}"), "superscripts");
        check(conv("r = √(mλR)").contains("\\sqrt{") && conv("a × b ≈ 30°").contains("\\times") && conv("a × b ≈ 30°").contains("^{\\circ}"), "roots and symbols");
        check(conv("\\frac{a}{b}") == "\\frac{a}{b}", "LaTeX input is left unchanged");
        check(conv("asin(x)").contains("\\arcsin"), "asin becomes arcsin");
        for (const char* f : {"n₁ sin θ₁ = n₂ sin θ₂", "θc = asin(n₂ / n₁)", "1/f = 1/dₒ + 1/dᵢ", "m = -dᵢ/dₒ = hᵢ/hₒ", "δ = i₁ + i₂ - A", "n = sin[(A+δmin)/2] / sin(A/2)", "β = λL/d  ·  yₘ = mλL/d", "I/I₀ = cos²(πdy/λL)", "s' > 0: real image", "|m| > 1"}) {
            QString err;
            check(LatexRenderer::validate(conv(f), &err), QString("reference formula renders: ") + f + " -> " + conv(f) + " " + err);
        }
    }

    // ---- Markdown highlighter: the specified colours, weights and sizes ----
    {
        const MarkdownPalette pal = MarkdownPalette::darkSet();
        auto first = [&](const QString& line, int state = 0, int* out = nullptr) {
            int s = 0;
            const auto r = GruvboxMarkdownHighlighter::analyze(line, state, out ? out : &s, pal);
            return r;
        };
        struct H { const char* text; const char* color; double pt; };
        for (const H& h : {H{"# Primary title", "#d79921", 18}, H{"## Chapter", "#e78a4e", 14}, H{"### Snell's law", "#a9b665", 12}, H{"#### Exam warning", "#ea696c", 11}}) {
            const auto r = first(h.text);
            check(r.size() == 1 && r[0].start == 0 && r[0].length == QString(h.text).size(), QString("heading covers the line: ") + h.text);
            if (r.size() == 1) {
                check(r[0].format.foreground().color() == QColor(h.color), QString("heading colour ") + h.color);
                check(r[0].format.fontWeight() == QFont::Bold, QString("heading bold: ") + h.text);
                check(std::fabs(r[0].format.fontPointSize() - h.pt) < 1e-9, QString("heading size %1 pt").arg(h.pt));
            }
        }
        check(GruvboxMarkdownHighlighter::headingLevel("##### deep") == 4 && GruvboxMarkdownHighlighter::headingLevel("#nospace") == 0 &&
              GruvboxMarkdownHighlighter::headingLevel("plain # text") == 0, "heading levels");

        auto mathColor = [&](const QString& line) {                       // text covered by an aqua monospace range
            auto ranges = first(line);
            std::sort(ranges.begin(), ranges.end(), [](const auto& a, const auto& b) { return a.start < b.start; });
            QStringList found;
            for (const auto& r : std::as_const(ranges))
                if (r.format.foreground().color() == QColor(0xb8, 0xbb, 0x26) && r.format.fontFamilies().toStringList().contains("JetBrains Mono"))
                    found << line.mid(r.start, r.length);
            return found;
        };
        const QStringList inlineMath = mathColor("The law $n₁ sin θ₁ = n₂ sin θ₂$ holds");
        check(inlineMath.contains("$n₁ sin θ₁ = n₂ sin θ₂$"), "inline math span is highlighted");
        const QStringList displayMath = mathColor("Result: $$\\frac{1}{f}$$");
        check(displayMath.contains("$$\\frac{1}{f}$$"), "display math span is highlighted");
    }

    std::cout << "UI checks: " << (g_failures == 0 ? "passed" : "failed") << "\n";
    return g_failures == 0 ? 0 : 1;
}
