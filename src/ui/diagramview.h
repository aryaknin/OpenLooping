#pragma once

#include <QGraphicsView>

class DiagramView : public QGraphicsView {
    Q_OBJECT
public:
    explicit DiagramView(QWidget *parent = nullptr);
    void setConnectionMode(bool active);

signals:
    void objectClicked(const QString &kind, const QString &id);
    void objectDoubleClicked(const QString &kind, const QString &id);
    void objectMoved(const QString &kind, const QString &id, const QPointF &position);
    void contextRequested(const QString &kind, const QString &id, const QPoint &globalPosition);
    void deleteRequested();
    void cancelRequested();
    void emptyClicked(const QPointF &position);
    void emptyDoubleClicked(const QPointF &position);
    void zoomSteps(int steps);

protected:
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    bool m_connectionMode = false;
};
