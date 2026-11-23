#include "SvgDiagramView.h"
#include "LanguageManager.h"
#include "ThemeManager.h"

#include <QFile>
#include <QImage>
#include <QRegularExpression>
#include <QPainter>
#include <QSizeF>

namespace phy104 {
namespace {

QString decodeXmlText(QString text) {
    text.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    text.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    text.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    text.replace(QStringLiteral("&apos;"), QStringLiteral("'"));
    text.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return text;
}

QString encodeXmlText(QString text) {
    text.replace(QLatin1Char('&'), QStringLiteral("&amp;"));
    text.replace(QLatin1Char('<'), QStringLiteral("&lt;"));
    text.replace(QLatin1Char('>'), QStringLiteral("&gt;"));
    return text;
}

QSizeF svgSize(const QSvgRenderer& renderer) {
    QSizeF size = renderer.defaultSize();
    if (size.width() <= 0 || size.height() <= 0) size = renderer.viewBoxF().size();
    return size;
}

QByteArray localizeSvgText(const QByteArray& svg) {
    if (!LanguageManager::instance().isArabic()) return svg;
    QString source = QString::fromUtf8(svg);
    const QRegularExpression textElement(QStringLiteral("(<text\\b[^>]*>)(.*?)(</text>)"),
                                         QRegularExpression::DotMatchesEverythingOption);
    QString output;
    qsizetype copied = 0;
    auto matches = textElement.globalMatch(source);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        output += source.mid(copied, match.capturedStart() - copied);
        const QString inner = match.captured(2);
        // Keep mathematical subscripts and inline SVG markup byte-for-byte. Their equations and notation are language-neutral.
        if (inner.contains(QStringLiteral("<tspan"))) {
            output += match.captured();
        } else {
            const QString decoded = decodeXmlText(inner);
            const QString translated = LanguageManager::instance().text(decoded);
            output += translated == decoded ? match.captured()
                                            : match.captured(1) + encodeXmlText(translated) + match.captured(3);
        }
        copied = match.capturedEnd();
    }
    output += source.mid(copied);
    return output.toUtf8();
}

} // namespace

SvgDiagramView::SvgDiagramView(QWidget* parent) : QWidget(parent) {
    setMinimumSize(360, 240);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] { reload(); });
}

QByteArray SvgDiagramView::applyTheme(QByteArray svg) {
    const Theme& t = ThemeManager::instance().theme();
    const std::pair<const char*, QColor> tokens[] = {
        {"@fg@", t.text},   {"@muted@", t.muted}, {"@red@", t.red},     {"@green@", t.green},
        {"@yellow@", t.yellow}, {"@orange@", t.orange}, {"@blue@", t.blue}, {"@aqua@", t.aqua},
        {"@card@", t.card}};
    for (const auto& [token, color] : tokens) svg.replace(token, color.name().toUtf8());

    // Keep every optical setup visually consistent instead of inheriting the
    // different serif/sans font declarations from older generated SVG files.
    QString normalized = QString::fromUtf8(svg);
    const QRegularExpression fontFamily(QStringLiteral("font-family\\s*=\\s*(['\\\"])[^'\\\"]*\\1"));
    normalized.replace(fontFamily, QStringLiteral("font-family=\"Inter, Noto Sans Arabic, DejaVu Sans, Arial, sans-serif\""));
    return normalized.toUtf8();
}

bool SvgDiagramView::load(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;
    m_path = path;
    m_template = file.readAll();
    reload();
    return m_renderer.isValid();
}

void SvgDiagramView::reload() {
    if (m_template.isEmpty()) return;
    m_loadedArabic = LanguageManager::instance().isArabic();
    m_renderer.load(applyTheme(localizeSvgText(m_template)));
    update();
}

void SvgDiagramView::paintEvent(QPaintEvent*) {
    if (!m_template.isEmpty() && m_loadedArabic != LanguageManager::instance().isArabic()) reload();
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    const Theme& t = ThemeManager::instance().theme();
    p.setPen(QPen(t.border, 1));
    p.setBrush(t.canvas);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
    if (!m_renderer.isValid()) return;

    const QSizeF native = svgSize(m_renderer);
    if (native.width() <= 0 || native.height() <= 0) return;
    const QRectF area = QRectF(rect()).adjusted(10, 10, -10, -10);
    const qreal scale = qMin(area.width() / native.width(), area.height() / native.height());
    const QSizeF size = native * scale;
    m_renderer.render(&p, QRectF(area.center().x() - size.width() / 2, area.center().y() - size.height() / 2,
                                 size.width(), size.height()));
}

bool SvgDiagramView::exportPng(const QString& fileName, int widthPixels) const {
    if (!m_renderer.isValid() || widthPixels <= 0) return false;
    const QSizeF native = svgSize(m_renderer);
    if (native.width() <= 0 || native.height() <= 0) return false;
    const int height = qMax(1, qRound(widthPixels * native.height() / native.width()));
    QImage image(widthPixels, height, QImage::Format_ARGB32);
    image.fill(ThemeManager::instance().theme().canvas);          // opaque, readable background
    QPainter p(&image);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    QSvgRenderer renderer(applyTheme(localizeSvgText(m_template))); // fresh renderer: render() is const-unfriendly
    renderer.render(&p, QRectF(0, 0, widthPixels, height));
    p.end();
    return image.save(fileName, "PNG");
}

} // namespace phy104
