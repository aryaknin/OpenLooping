#include "ui/diagramview.h"

#include <QGraphicsItem>
#include <QContextMenuEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

DiagramView::DiagramView(QWidget *parent) : QGraphicsView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setFocusPolicy(Qt::StrongFocus);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
}

void DiagramView::setConnectionMode(bool active)
{
    m_connectionMode = active;
    viewport()->setCursor(active ? Qt::CrossCursor : Qt::ArrowCursor);
}

void DiagramView::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor("#f7f8fa"));
    const int grid = 25;
    const int majorGrid = 100;
    const int left = qFloor(rect.left() / grid) * grid;
    const int top = qFloor(rect.top() / grid) * grid;
    QVector<QLineF> minorLines;
    QVector<QLineF> majorLines;
    for (int x = left; x <= rect.right(); x += grid)
        (x % majorGrid == 0 ? majorLines : minorLines).append(QLineF(x, rect.top(), x, rect.bottom()));
    for (int y = top; y <= rect.bottom(); y += grid)
        (y % majorGrid == 0 ? majorLines : minorLines).append(QLineF(rect.left(), y, rect.right(), y));
    if (transform().m11() >= 0.55) {
        painter->setPen(QPen(QColor("#eceef1"), 0));
        painter->drawLines(minorLines);
    }
    painter->setPen(QPen(QColor("#e1e4e8"), 0));
    painter->drawLines(majorLines);
}

void DiagramView::mousePressEvent(QMouseEvent *event)
{
    if (m_connectionMode && event->button() == Qt::LeftButton) {
        if (auto *item = itemAt(event->position().toPoint())) {
            while (item->parentItem()) item = item->parentItem();
            emit objectClicked(item->data(0).toString(), item->data(1).toString());
        } else {
            emit emptyClicked(mapToScene(event->position().toPoint()));
        }
        event->accept();
        return;
    }
    if (auto *item = itemAt(event->position().toPoint())) {
        while (item->parentItem()) item = item->parentItem();
        emit objectClicked(item->data(0).toString(), item->data(1).toString());
    } else emit emptyClicked(mapToScene(event->position().toPoint()));
    QGraphicsView::mousePressEvent(event);
}

void DiagramView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (auto *item = itemAt(event->position().toPoint())) {
        while (item->parentItem()) item = item->parentItem();
        emit objectDoubleClicked(item->data(0).toString(), item->data(1).toString());
    } else {
        emit emptyDoubleClicked(mapToScene(event->position().toPoint()));
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void DiagramView::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    for (auto *item : scene()->selectedItems()) {
        while (item->parentItem()) item = item->parentItem();
        const QString kind = item->data(0).toString();
        if (kind == "entity" || kind == "association" || kind == "inheritance" || kind == "rule")
            emit objectMoved(kind, item->data(1).toString(), item->scenePos());
    }
}

void DiagramView::contextMenuEvent(QContextMenuEvent *event)
{
    if (auto *item = itemAt(event->pos())) {
        while (item->parentItem()) item = item->parentItem();
        emit contextRequested(item->data(0).toString(), item->data(1).toString(), event->globalPos());
        return;
    }
    QGraphicsView::contextMenuEvent(event);
}

void DiagramView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit cancelRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        emit deleteRequested();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void DiagramView::wheelEvent(QWheelEvent *event)
{
    const int steps = event->angleDelta().y() / 120;
    if (steps != 0) {
        emit zoomSteps(steps);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}
