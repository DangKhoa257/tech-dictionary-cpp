#pragma once

#include <functional>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>

namespace IconHelper {

inline QIcon renderIcon(int width, int height, const std::function<void(QPainter&, qreal, qreal)>& draw) {
    constexpr qreal scale = 2.0;
    QPixmap pixmap(static_cast<int>(width * scale), static_cast<int>(height * scale));
    pixmap.fill(Qt::transparent);

    {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.scale(scale, scale);
        draw(painter, width, height);
    }

    return QIcon(pixmap);
}

inline QPixmap renderPixmap(int width, int height, const std::function<void(QPainter&, qreal, qreal)>& draw) {
    constexpr qreal scale = 2.0;
    QPixmap pixmap(static_cast<int>(width * scale), static_cast<int>(height * scale));
    pixmap.fill(Qt::transparent);

    {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.scale(scale, scale);
        draw(painter, width, height);
    }

    return pixmap;
}

// Open Book Icon
inline QIcon bookIcon(const QColor& color = QColor(QStringLiteral("#2563EB")), int size = 20) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal mx = w * 0.5;
        qreal top = h * 0.22;
        qreal bot = h * 0.78;
        qreal left = w * 0.16;
        qreal right = w * 0.84;

        // Spine
        p.drawLine(QPointF(mx, top + 2), QPointF(mx, bot));

        // Left page
        QPainterPath leftPath;
        leftPath.moveTo(mx, top + 2);
        leftPath.cubicTo(mx - 4, top, left + 4, top, left, top + 2);
        leftPath.lineTo(left, bot);
        leftPath.cubicTo(left + 4, bot - 2, mx - 4, bot - 2, mx, bot);
        p.drawPath(leftPath);

        // Right page
        QPainterPath rightPath;
        rightPath.moveTo(mx, top + 2);
        rightPath.cubicTo(mx + 4, top, right - 4, top, right, top + 2);
        rightPath.lineTo(right, bot);
        rightPath.cubicTo(right - 4, bot - 2, mx + 4, bot - 2, mx, bot);
        p.drawPath(rightPath);
    });
}

// 4-Squares Grid Icon (for "Tất cả từ")
inline QIcon gridIcon(const QColor& color = QColor(QStringLiteral("#FFFFFF")), int size = 20) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        p.setPen(Qt::NoPen);
        p.setBrush(color);

        qreal boxSize = w * 0.30;
        qreal gap = w * 0.12;
        qreal offset = (w - (2 * boxSize + gap)) / 2.0;

        p.drawRoundedRect(QRectF(offset, offset, boxSize, boxSize), 2, 2);
        p.drawRoundedRect(QRectF(offset + boxSize + gap, offset, boxSize, boxSize), 2, 2);
        p.drawRoundedRect(QRectF(offset, offset + boxSize + gap, boxSize, boxSize), 2, 2);
        p.drawRoundedRect(QRectF(offset + boxSize + gap, offset + boxSize + gap, boxSize, boxSize), 2, 2);
    });
}

// Folder Icon
inline QIcon folderIcon(const QColor& color = QColor(QStringLiteral("#94A3B8")), int size = 20) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal left = w * 0.15;
        qreal right = w * 0.85;
        qreal top = h * 0.28;
        qreal bot = h * 0.76;
        qreal tabW = w * 0.32;

        QPainterPath path;
        path.moveTo(left, bot);
        path.lineTo(left, top);
        path.lineTo(left + tabW, top);
        path.lineTo(left + tabW + 3, top + 4);
        path.lineTo(right, top + 4);
        path.lineTo(right, bot);
        path.closeSubpath();
        p.drawPath(path);
    });
}

// Magnifying Glass Search Icon
inline QIcon searchIcon(const QColor& color = QColor(QStringLiteral("#64748B")), int size = 20) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal cx = w * 0.42;
        qreal cy = h * 0.42;
        qreal radius = w * 0.26;

        p.drawEllipse(QPointF(cx, cy), radius, radius);
        p.drawLine(QPointF(cx + radius * 0.7, cy + radius * 0.7), QPointF(w * 0.82, h * 0.82));
    });
}

// Laptop / Monitor Icon (Header)
inline QPixmap laptopPixmap(const QColor& color = QColor(QStringLiteral("#2563EB")), int size = 32) {
    return renderPixmap(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        // Screen
        qreal screenLeft = w * 0.20;
        qreal screenRight = w * 0.80;
        qreal screenTop = h * 0.22;
        qreal screenBot = h * 0.65;
        p.drawRoundedRect(QRectF(screenLeft, screenTop, screenRight - screenLeft, screenBot - screenTop), 3, 3);

        // Base
        qreal baseLeft = w * 0.12;
        qreal baseRight = w * 0.88;
        qreal baseBot = h * 0.76;
        p.drawLine(QPointF(baseLeft, baseBot), QPointF(baseRight, baseBot));
    });
}

// Layers / Category Icon
inline QPixmap layersPixmap(const QColor& color = QColor(QStringLiteral("#7C3AED")), int size = 26) {
    return renderPixmap(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal cx = w * 0.5;
        // Top diamond
        QPainterPath top;
        top.moveTo(cx, h * 0.20);
        top.lineTo(w * 0.82, h * 0.35);
        top.lineTo(cx, h * 0.50);
        top.lineTo(w * 0.18, h * 0.35);
        top.closeSubpath();
        p.drawPath(top);

        // Middle layer
        QPainterPath mid;
        mid.moveTo(w * 0.18, h * 0.52);
        mid.lineTo(cx, h * 0.66);
        mid.lineTo(w * 0.82, h * 0.52);
        p.drawPath(mid);

        // Bottom layer
        QPainterPath bot;
        bot.moveTo(w * 0.18, h * 0.68);
        bot.lineTo(cx, h * 0.82);
        bot.lineTo(w * 0.82, h * 0.68);
        p.drawPath(bot);
    });
}

// Lightning / Speed Icon
inline QPixmap lightningPixmap(const QColor& color = QColor(QStringLiteral("#059669")), int size = 26) {
    return renderPixmap(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(color);

        QPainterPath path;
        path.moveTo(w * 0.55, h * 0.15);
        path.lineTo(w * 0.25, h * 0.52);
        path.lineTo(w * 0.48, h * 0.52);
        path.lineTo(w * 0.42, h * 0.85);
        path.lineTo(w * 0.75, h * 0.45);
        path.lineTo(w * 0.52, h * 0.45);
        path.closeSubpath();
        p.drawPath(path);
    });
}

// Brand Logo Box (Tech Dictionary)
inline QPixmap brandLogoPixmap(int size = 42) {
    return renderPixmap(size, size, [](QPainter& p, qreal w, qreal h) {
        // Gradient rounded background
        QLinearGradient grad(0, 0, w, h);
        grad.setColorAt(0.0, QColor(QStringLiteral("#3B82F6")));
        grad.setColorAt(1.0, QColor(QStringLiteral("#1D4ED8")));
        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawRoundedRect(QRectF(0, 0, w, h), 10, 10);

        // White open book inside
        QPen pen(QColor(QStringLiteral("#FFFFFF")), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal mx = w * 0.5;
        qreal top = h * 0.26;
        qreal bot = h * 0.74;
        qreal left = w * 0.20;
        qreal right = w * 0.80;

        p.drawLine(QPointF(mx, top + 2), QPointF(mx, bot));

        QPainterPath lp;
        lp.moveTo(mx, top + 2);
        lp.cubicTo(mx - 4, top, left + 4, top, left, top + 2);
        lp.lineTo(left, bot);
        lp.cubicTo(left + 4, bot - 2, mx - 4, bot - 2, mx, bot);
        p.drawPath(lp);

        QPainterPath rp;
        rp.moveTo(mx, top + 2);
        rp.cubicTo(mx + 4, top, right - 4, top, right, top + 2);
        rp.lineTo(right, bot);
        rp.cubicTo(right - 4, bot - 2, mx + 4, bot - 2, mx, bot);
        p.drawPath(rp);
    });
}

// Edit (Pencil) Icon
inline QIcon editIcon(const QColor& color = QColor(QStringLiteral("#2563EB")), int size = 18) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        QPainterPath path;
        path.moveTo(w * 0.20, h * 0.80);
        path.lineTo(w * 0.20, h * 0.65);
        path.lineTo(w * 0.65, h * 0.20);
        path.lineTo(w * 0.80, h * 0.35);
        path.lineTo(w * 0.35, h * 0.80);
        path.closeSubpath();
        p.drawPath(path);

        p.drawLine(QPointF(w * 0.55, h * 0.30), QPointF(w * 0.70, h * 0.45));
    });
}

// Delete (Trash) Icon
inline QIcon deleteIcon(const QColor& color = QColor(QStringLiteral("#DC2626")), int size = 18) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal left = w * 0.24;
        qreal right = w * 0.76;
        qreal top = h * 0.32;
        qreal bot = h * 0.82;

        // Lid
        p.drawLine(QPointF(w * 0.18, top), QPointF(w * 0.82, top));
        p.drawLine(QPointF(w * 0.38, top - 3), QPointF(w * 0.62, top - 3));

        // Can
        QPainterPath can;
        can.moveTo(left, top);
        can.lineTo(left + 2, bot);
        can.lineTo(right - 2, bot);
        can.lineTo(right, top);
        p.drawPath(can);

        // Lines
        p.drawLine(QPointF(w * 0.42, top + 4), QPointF(w * 0.42, bot - 4));
        p.drawLine(QPointF(w * 0.58, top + 4), QPointF(w * 0.58, bot - 4));
    });
}

// Dictionary Lookup / Home Icon (Sidebar top)
inline QIcon lookupIcon(const QColor& color = QColor(QStringLiteral("#FFFFFF")), int size = 20) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        // Home outline
        QPainterPath house;
        house.moveTo(w * 0.16, h * 0.46);
        house.lineTo(w * 0.50, h * 0.20);
        house.lineTo(w * 0.84, h * 0.46);
        house.lineTo(w * 0.84, h * 0.80);
        house.lineTo(w * 0.16, h * 0.80);
        house.closeSubpath();
        p.drawPath(house);
    });
}

// Info Icon
inline QIcon infoIcon(const QColor& color = QColor(QStringLiteral("#94A3B8")), int size = 16) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        qreal r = w * 0.38;
        p.drawEllipse(QPointF(w * 0.5, h * 0.5), r, r);
        p.drawPoint(QPointF(w * 0.5, h * 0.36));
        p.drawLine(QPointF(w * 0.5, h * 0.48), QPointF(w * 0.5, h * 0.66));
    });
}

// Version Tag Icon
inline QIcon tagIcon(const QColor& color = QColor(QStringLiteral("#94A3B8")), int size = 16) {
    return renderIcon(size, size, [color](QPainter& p, qreal w, qreal h) {
        QPen pen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);

        QPainterPath path;
        path.moveTo(w * 0.20, h * 0.40);
        path.lineTo(w * 0.40, h * 0.20);
        path.lineTo(w * 0.80, h * 0.20);
        path.lineTo(w * 0.80, h * 0.60);
        path.lineTo(w * 0.60, h * 0.80);
        path.lineTo(w * 0.20, h * 0.80);
        path.closeSubpath();
        p.drawPath(path);
    });
}

}  // namespace IconHelper
