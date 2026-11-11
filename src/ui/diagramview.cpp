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
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
}

void DiagramView::setConnectionMode(bool active)
{
    m_connectionMode = active;
    if (!active) clearConnectionStart();
    viewport()->setCursor(active ? Qt::CrossCursor : Qt::ArrowCursor);
    viewport()->update();
}

void DiagramView::setConnectionStart(const QString &kind, const QString &id)
{
    m_connectionStartKind = kind;
    m_connectionStartId = id;
    viewport()->update();
}

void DiagramView::clearConnectionStart()
{
    m_connectionStartKind.clear();
    m_connectionStartId.clear();
    m_hasConnectionCursor = false;
    viewport()->update();
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

QGraphicsItem *DiagramView::connectionItem(const QString &kind, const QString &id) const
{
    if (!scene()) return nullptr;
    for (auto *item : scene()->items()) {
        while (item->parentItem()) item = item->parentItem();
        if (item->data(0).toString() == kind && item->data(1).toString() == id)
            return item;
    }
    return nullptr;
}

QGraphicsItem *DiagramView::compatibleTargetAt(const QPoint &viewportPosition) const
{
    auto *item = itemAt(viewportPosition);
    if (!item) return nullptr;
    while (item->parentItem()) item = item->parentItem();
    const QString kind = item->data(0).toString();
    const bool startIsEntity = m_connectionStartKind == QStringLiteral("entity");
    const bool compatible = startIsEntity
        ? kind == QStringLiteral("association") || kind == QStringLiteral("inheritance")
        : kind == QStringLiteral("entity");
    return compatible ? item : nullptr;
}

void DiagramView::drawForeground(QPainter *painter, const QRectF &rect)
{
    QGraphicsView::drawForeground(painter, rect);
    if (!m_connectionMode || m_connectionStartId.isEmpty()) return;

    auto *start = connectionItem(m_connectionStartKind, m_connectionStartId);
    if (!start) return;
    const QRectF startRect = start->sceneBoundingRect().adjusted(-5, -5, 5, 5);

    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(QStringLiteral("#2563eb")), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawRoundedRect(startRect, 7, 7);

    if (m_hasConnectionCursor) {
        QPointF targetPoint = m_connectionCursor;
        if (auto *target = compatibleTargetAt(mapFromScene(m_connectionCursor))) {
            const QRectF targetRect = target->sceneBoundingRect().adjusted(-5, -5, 5, 5);
            targetPoint = target->sceneBoundingRect().center();
            painter->setPen(QPen(QColor(QStringLiteral("#16a34a")), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->drawRoundedRect(targetRect, 7, 7);
        } else {
            painter->setPen(QPen(QColor(QStringLiteral("#2563eb")), 2.2, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        }
        painter->drawLine(start->sceneBoundingRect().center(), targetPoint);
        painter->setBrush(painter->pen().color());
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(targetPoint, 3.5, 3.5);
    }
    painter->restore();
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

void DiagramView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_connectionMode) {
        m_connectionCursor = mapToScene(event->position().toPoint());
        m_hasConnectionCursor = true;
        viewport()->update();
    }
    QGraphicsView::mouseMoveEvent(event);
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
    emit repeatToolRequested();
    event->accept();
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
