#include "Icons.h"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

namespace phy104 {

QPixmap Icons::pixmap(const QString& name, const QColor& color, int size, qreal dpr) {
    if (size <= 0) return {};
    QFile file(":/icons/" + name + ".svg");
    if (!file.open(QIODevice::ReadOnly)) return {};
    QByteArray svg = file.readAll();
    if (!color.isValid()) return {};
    // SVG 2's currentColor is not supported consistently by all QtSvg versions.
    svg.replace("currentColor", color.name(QColor::HexRgb).toUtf8());
    QSvgRenderer renderer(svg);
    if (!renderer.isValid()) return {};

    dpr = qMax<qreal>(1.0, dpr);
    const int pixelSize = qMax(1, qRound(size * dpr));
    QPixmap pix(pixelSize, pixelSize);
    pix.setDevicePixelRatio(dpr);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    renderer.render(&p, QRectF(0, 0, size, size));
    return pix;
}

QIcon Icons::get(const QString& name, const QColor& color, const QColor& colorOn, int size) {
    QIcon icon;
    icon.addPixmap(pixmap(name, color, size), QIcon::Normal, QIcon::Off);
    icon.addPixmap(pixmap(name, colorOn.isValid() ? colorOn : color, size), QIcon::Normal, QIcon::On);
    return icon;
}

} // namespace phy104
