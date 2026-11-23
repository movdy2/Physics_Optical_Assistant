#include "Plots.h"
#include "LanguageManager.h"
#include "ThemeManager.h"

#include <QFontMetricsF>
#include <QPainter>

#include <algorithm>
#include <cmath>

namespace phy104 {
namespace {
QString displayText(const QString& source) {
    return LanguageManager::instance().text(source);
}

void drawIsolatedText(QPainter& painter, const QPointF& baseline, const QString& value) {
    bool containsArabic = false;
    for (const QChar ch : value) {
        const ushort code = ch.unicode();
        if ((code >= 0x0600 && code <= 0x08FF) || (code >= 0xFB50 && code <= 0xFEFF)) {
            containsArabic = true;
            break;
        }
    }
    if (containsArabic) {
        QString isolated;
        bool inLtrRun = false;
        for (const QChar ch : value) {
            const ushort code = ch.unicode();
            const bool isArabic = (code >= 0x0600 && code <= 0x08FF) || (code >= 0xFB50 && code <= 0xFEFF);
            if (!isArabic && !inLtrRun) {
                isolated.append(QChar(ushort(0x2066))); // LRI: keep Latin/math runs ordered inside RTL labels
                inLtrRun = true;
            } else if (isArabic && inLtrRun) {
                isolated.append(QChar(ushort(0x2069))); // PDI
                inLtrRun = false;
            }
            isolated.append(ch);
        }
        if (inLtrRun) isolated.append(QChar(ushort(0x2069)));
        painter.drawText(baseline, isolated);
        return;
    }
    const QFontMetricsF fm(painter.font());
    const QRectF bounds(baseline.x(), baseline.y() - fm.ascent(), fm.horizontalAdvance(value) + 2.0, fm.height() + 1.0);
    painter.drawText(bounds, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine | Qt::TextForceLeftToRight, value);
}
} // namespace

GridCanvas::GridCanvas(QWidget* parent) : QWidget(parent) {
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] { update(); });
}

void GridCanvas::paintEvent(QPaintEvent*) {
    const Theme& t = ThemeManager::instance().theme();
    QPainter p(this);
    if (m_antialiased)
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    p.fillRect(rect(), t.canvas);
    p.save();
    paintDiagram(p, QRectF(rect()));
    p.restore();
}

// ---------------------------------------------------------------------------------- LineChart
LineChart::LineChart(QWidget* parent) : QWidget(parent) {
    setMinimumHeight(132);
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this] { update(); });
}

void LineChart::setAxes(double xMin, double xMax, int xTicks, int xDecimals, const QString& xLabel,
                        double yMin, double yMax, int yTicks, int yDecimals, const QString& yLabel) {
    m_xMin = xMin; m_xMax = xMax; m_xTicks = xTicks; m_xDec = xDecimals; m_xLabel = xLabel;
    m_yMin = yMin; m_yMax = yMax; m_yTicks = yTicks; m_yDec = yDecimals; m_yLabel = yLabel;
    update();
}

void LineChart::setLegend(const QStringList& items, const QString& caption) {
    m_legend = items;
    m_caption = caption;
    update();
}

void LineChart::setMarkerX(double x, bool enabled) {
    m_markerX = x;
    m_hasMarker = enabled;
    update();
}

void LineChart::setAnnotations(const QList<ChartVLine>& v, const QList<ChartHLine>& hl, const QList<ChartRegion>& r, const QList<ChartPoint>& p) {
    m_vlines = v; m_hlines = hl; m_regions = r; m_points = p;
    update();
}

void LineChart::paintEvent(QPaintEvent*) {
    const Theme& t = ThemeManager::instance().theme();
    QPainter p(this);
    if (m_antialiased)
        p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    p.fillRect(rect(), t.canvas);

    QFont mono(ThemeManager::monoFont());
    mono.setPixelSize(9);
    p.setFont(mono);
    const QFontMetricsF fm(mono);

    struct LegendDrawItem { QString label; bool marker = false; int row = 0; double x = 0.0; };
    QList<LegendDrawItem> legendItems;
    const double legendLeft = 46.0;
    const double legendRight = std::max(legendLeft + 1.0, double(width()) - 10.0);
    const double legendWidth = std::max(1.0, legendRight - legendLeft);
    double legendX = legendLeft;
    int legendRow = 0;
    auto placeLegendItem = [&](const QString& source, bool marker) {
        if (source.isEmpty()) return;
        QString label = displayText(source);
        const double markerWidth = marker ? 24.0 : 0.0;
        const double maxTextWidth = std::max(1.0, legendWidth - markerWidth);
        label = fm.elidedText(label, Qt::ElideRight, static_cast<int>(std::floor(maxTextWidth)));
        double itemWidth = markerWidth + fm.horizontalAdvance(label) + 10.0;
        if (legendX > legendLeft && legendX + itemWidth > legendRight && legendRow == 0) {
            legendRow = 1;
            legendX = legendLeft;
        }
        const double remaining = std::max(1.0, legendRight - legendX - markerWidth - 8.0);
        if (legendX + itemWidth > legendRight) {
            label = fm.elidedText(label, Qt::ElideRight, static_cast<int>(std::floor(remaining)));
            itemWidth = markerWidth + fm.horizontalAdvance(label) + 8.0;
        }
        legendItems.push_back({label, marker, legendRow, legendX});
        legendX = std::min(legendRight, legendX + itemWidth + 8.0);
    };
    for (const QString& item : m_legend) placeLegendItem(item, true);
    if (!m_caption.isEmpty()) placeLegendItem(m_caption, false);
    const int legendRows = legendItems.isEmpty() ? 0 : legendRow + 1;
    const double lowerReserve = legendRows == 0 ? 42.0 : (legendRows == 1 ? 54.0 : 68.0);
    const QRectF plot(46, 24, std::max(1, width() - 64), std::max(1, height() - 24 - lowerReserve));
    auto X = [&](double x) { return plot.left() + (x - m_xMin) / (m_xMax - m_xMin) * plot.width(); };
    auto Y = [&](double y) { return plot.bottom() - (y - m_yMin) / (m_yMax - m_yMin) * plot.height(); };

    const QString yLabel = fm.elidedText(displayText(m_yLabel), Qt::ElideRight, 38);
    const QString xLabel = fm.elidedText(displayText(m_xLabel), Qt::ElideRight, std::max(1, static_cast<int>(plot.width())));
    p.setPen(t.dim);
    drawIsolatedText(p, QPointF(8, 14), yLabel);
    drawIsolatedText(p, QPointF(plot.right() - fm.horizontalAdvance(xLabel), plot.bottom() + 30), xLabel);

    for (const ChartRegion& r : m_regions) {                              // shaded regions behind the data
        QColor fill = r.color.isValid() ? r.color : t.amber;
        fill.setAlpha(34);
        p.fillRect(QRectF(X(r.x0), plot.top(), X(r.x1) - X(r.x0), plot.height()), fill);
        if (!r.label.isEmpty()) {
            p.setPen(r.color.isValid() ? r.color : t.amber);
            drawIsolatedText(p, QPointF(X(r.x0) + 8, plot.top() + 28), displayText(r.label));
        }
    }
    p.setPen(QPen(t.border, 1));
    for (int i = 0; i <= m_xTicks; ++i) {                                 // x tick labels
        const double v = m_xMin + (m_xMax - m_xMin) * i / m_xTicks;
        p.setPen(t.dim);
        const QString s = QString::number(v, 'f', m_xDec);
        drawIsolatedText(p, QPointF(X(v) - fm.horizontalAdvance(s) / 2, plot.bottom() + 14), s);
    }
    for (int i = 0; i <= m_yTicks; ++i) {                                 // y tick labels
        const double v = m_yMin + (m_yMax - m_yMin) * i / m_yTicks;
        p.setPen(t.dim);
        const QString s = QString::number(v, 'f', m_yDec);
        drawIsolatedText(p, QPointF(plot.left() - 6 - fm.horizontalAdvance(s), Y(v) + 3), s);
    }
    p.setPen(QPen(t.border, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot);

    p.save();
    p.setClipRect(plot.adjusted(-2, -2, 2, 2));
    if (m_hasMarker) {
        p.setPen(QPen(t.dim, 1, Qt::DotLine));
        p.drawLine(QPointF(X(m_markerX), plot.top()), QPointF(X(m_markerX), plot.bottom()));
    }
    for (const ChartHLine& h : m_hlines) {
        p.setPen(QPen(h.color.isValid() ? h.color : t.dim, 1, Qt::DashLine));
        p.drawLine(QPointF(plot.left(), Y(h.y)), QPointF(plot.right(), Y(h.y)));
        p.setPen(t.muted);
        const QString label = displayText(h.label);
        drawIsolatedText(p, QPointF(plot.right() - 8 - fm.horizontalAdvance(label), Y(h.y) - 6), label);
    }
    for (const ChartVLine& v : m_vlines) {
        p.setPen(QPen(v.color.isValid() ? v.color : t.text, 1.4, Qt::DashLine));
        p.drawLine(QPointF(X(v.x), plot.top()), QPointF(X(v.x), plot.bottom()));
    }
    for (const ChartSeries& s : m_series) {
        if (s.points.isEmpty()) continue;
        if (s.line) {
            QPolygonF poly;
            for (const QPointF& pt : s.points) poly << QPointF(X(pt.x()), Y(pt.y()));
            p.setPen(QPen(s.color.isValid() ? s.color : t.amber, 1.8));
            p.drawPolyline(poly);
        }
        if (!s.xUncertainties.isEmpty() || !s.yUncertainties.isEmpty()) {
            QColor errorColor = s.color.isValid() ? s.color : (s.markers ? t.sage : t.amber);
            errorColor.setAlpha(210);
            p.setPen(QPen(errorColor, 1.0));
            p.setBrush(Qt::NoBrush);
            for (qsizetype i = 0; i < s.points.size(); ++i) {
                const QPointF& point = s.points[i];
                const double xSigma = i < s.xUncertainties.size() ? s.xUncertainties[i] : 0.0;
                const double ySigma = i < s.yUncertainties.size() ? s.yUncertainties[i] : 0.0;
                if (std::isfinite(xSigma) && xSigma > 0.0) {
                    const double left = X(point.x() - xSigma), right = X(point.x() + xSigma), y = Y(point.y());
                    p.drawLine(QPointF(left, y), QPointF(right, y));
                    p.drawLine(QPointF(left, y - 3), QPointF(left, y + 3));
                    p.drawLine(QPointF(right, y - 3), QPointF(right, y + 3));
                }
                if (std::isfinite(ySigma) && ySigma > 0.0) {
                    const double top = Y(point.y() + ySigma), bottom = Y(point.y() - ySigma), x = X(point.x());
                    p.drawLine(QPointF(x, top), QPointF(x, bottom));
                    p.drawLine(QPointF(x - 3, top), QPointF(x + 3, top));
                    p.drawLine(QPointF(x - 3, bottom), QPointF(x + 3, bottom));
                }
            }
        }
        if (s.markers) {
            p.setPen(Qt::NoPen);
            p.setBrush(s.color.isValid() ? s.color : t.sage);
            for (const QPointF& pt : s.points) p.drawEllipse(QPointF(X(pt.x()), Y(pt.y())), 2.6, 2.6);
        }
    }
    p.restore();

    for (const ChartVLine& v : m_vlines) {                                 // labels are drawn outside the clip
        if (v.label.isEmpty()) continue;
        p.setPen(v.color.isValid() ? v.color : t.text);
        const QString label = displayText(v.label);
        const double lw = fm.horizontalAdvance(label);
        drawIsolatedText(p, QPointF(std::min(plot.right() - lw, std::max(plot.left(), X(v.x) - lw / 2)), v.labelAtTop ? plot.top() - 6 : plot.bottom() - 18), label);
    }
    for (const ChartPoint& pt : m_points) {
        const QColor col = pt.color.isValid() ? pt.color : t.sage;
        if (pt.xUncertainty > 0.0 || pt.yUncertainty > 0.0) {
            p.save();
            p.setClipRect(plot.adjusted(-2, -2, 2, 2));
            p.setPen(QPen(col, 1.1));
            if (std::isfinite(pt.xUncertainty) && pt.xUncertainty > 0.0) {
                const double left = X(pt.x - pt.xUncertainty), right = X(pt.x + pt.xUncertainty), y = Y(pt.y);
                p.drawLine(QPointF(left, y), QPointF(right, y));
                p.drawLine(QPointF(left, y - 3), QPointF(left, y + 3));
                p.drawLine(QPointF(right, y - 3), QPointF(right, y + 3));
            }
            if (std::isfinite(pt.yUncertainty) && pt.yUncertainty > 0.0) {
                const double top = Y(pt.y + pt.yUncertainty), bottom = Y(pt.y - pt.yUncertainty), x = X(pt.x);
                p.drawLine(QPointF(x, top), QPointF(x, bottom));
                p.drawLine(QPointF(x - 3, top), QPointF(x + 3, top));
                p.drawLine(QPointF(x - 3, bottom), QPointF(x + 3, bottom));
            }
            p.restore();
        }
        if (pt.crosshair) {
            p.setPen(QPen(t.dim, 1, Qt::DashLine));
            p.drawLine(QPointF(X(pt.x), Y(pt.y)), QPointF(X(pt.x), plot.bottom()));
            p.drawLine(QPointF(plot.left(), Y(pt.y)), QPointF(X(pt.x), Y(pt.y)));
        }
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawEllipse(QPointF(X(pt.x), Y(pt.y)), 4, 4);
        if (!pt.label.isEmpty()) {
            p.setPen(t.text);
            const QString label = displayText(pt.label);
            const double lw = fm.horizontalAdvance(label);
            drawIsolatedText(p, QPointF(std::min(plot.right() - lw - 4, X(pt.x) + 10), Y(pt.y) - 8), label);
        }
    }

    // Legend gets its own reserved rows below the x-axis title. Long labels are elided
    // inside their assigned space; wrapping to a second row avoids collisions at narrow widths.
    for (const LegendDrawItem& item : legendItems) {
        const double y = height() - 8.0 - (legendRows - 1 - item.row) * 14.0;
        if (item.marker) {
            p.setPen(Qt::NoPen);
            p.setBrush(t.sage);
            p.drawEllipse(QPointF(item.x + 3.0, y - 3.0), 3.0, 3.0);
        }
        p.setPen(t.muted);
        drawIsolatedText(p, QPointF(item.x + (item.marker ? 12.0 : 0.0), y), item.label);
    }
}

} // namespace phy104
