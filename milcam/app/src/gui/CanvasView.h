// milcam/app/src/gui/CanvasView.h
#pragma once
#include <QGraphicsView>
class QGraphicsScene;

// 2D top-view canvas. Draws a light grid background now; real stock/contours/
// toolpaths arrive in P1/P3. drawBackground paints the grid in scene coords.
class CanvasView : public QGraphicsView {
    Q_OBJECT
public:
    explicit CanvasView(QWidget* parent = nullptr);
protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
private:
    QGraphicsScene* scene_ = nullptr;
};
