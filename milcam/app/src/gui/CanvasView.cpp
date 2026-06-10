// milcam/app/src/gui/CanvasView.cpp
#include "gui/CanvasView.h"
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QPainter>
#include <cmath>

CanvasView::CanvasView(QWidget* parent) : QGraphicsView(parent) {
    setObjectName("Canvas");
    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-200, -200, 400, 400);
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing, true);

    auto* hint = scene_->addText("Import a DXF or add a shape");
    hint->setDefaultTextColor(QColor("#9AA7B8"));
    hint->setPos(-110, -10);
}

void CanvasView::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, QColor("#FCFCFD"));
    QPen pen(QColor("#E2E8F0"));
    pen.setCosmetic(true);
    painter->setPen(pen);
    const int step = 20;
    const int l = int(std::floor(rect.left() / step)) * step;
    const int t = int(std::floor(rect.top() / step)) * step;
    for (int x = l; x < rect.right(); x += step)
        painter->drawLine(x, int(rect.top()), x, int(rect.bottom()));
    for (int y = t; y < rect.bottom(); y += step)
        painter->drawLine(int(rect.left()), y, int(rect.right()), y);
    // axes
    QPen axis(QColor("#C2CDDC")); axis.setCosmetic(true); axis.setWidth(2);
    painter->setPen(axis);
    painter->drawLine(int(rect.left()), 0, int(rect.right()), 0);
    painter->drawLine(0, int(rect.top()), 0, int(rect.bottom()));
}
