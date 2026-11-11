#pragma once

#include "core/model.h"

#include <QMainWindow>

class DiagramView;
class QGraphicsScene;
class QPlainTextEdit;
class QLabel;
class QSlider;
class QDockWidget;
class QCloseEvent;
class QMdiArea;
class QMdiSubWindow;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();
    bool loadModel(const QString &path, bool showWarnings = true);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newModel();
    void openModel();
    bool saveModel();
    bool saveModelAs();
    void exportSql();
    void exportMld();
    void addEntity();
    void addAssociation();
    void addInheritance();
    void addRule();
    void startLink();
    void cancelCurrentTool();
    void objectClicked(const QString &kind, const QString &id);
    void editObject(const QString &kind, const QString &id);
    void placeObject(const QPointF &position);
    void moveObject(const QString &kind, const QString &id, const QPointF &position);
    void showObjectMenu(const QString &kind, const QString &id, const QPoint &globalPosition);
    void deleteSelected();
    void undo();
    void redo();
    void cutSelected();
    void copySelected();
    void pasteClipboard();
    void duplicateSelected();
    void selectAllObjects();

private:
    void setupUi();
    void rebuildScene();
    void updateGeneratedOutput();
    void fitModelToView();
    bool writeModel(const QString &path);
    void setDirty(bool dirty = true);
    void checkpoint();
    bool confirmDiscard();

    looping::Model m_model;
    QString m_path;
    bool m_dirty = false;
    bool m_linkMode = false;
    bool m_showFields = true;
    bool m_showTypes = false;
    QString m_linkStartKind;
    QString m_linkStartId;
    QString m_pendingPlacement;
    QVector<looping::Model> m_undo;
    QVector<looping::Model> m_redo;
    looping::Model m_clipboard;
    int m_pasteGeneration = 0;
    DiagramView *m_view;
    QGraphicsScene *m_scene;
    QPlainTextEdit *m_mld;
    QPlainTextEdit *m_sql;
    QMdiArea *m_mdiArea;
    QMdiSubWindow *m_documentWindow;
    QDockWidget *m_mldDock;
    QDockWidget *m_sqlDock;
    QLabel *m_zoomLabel;
    QSlider *m_zoomSlider;
};
