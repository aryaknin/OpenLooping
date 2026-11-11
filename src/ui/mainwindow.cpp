#include "ui/mainwindow.h"

#include "core/persistence.h"
#include "core/relationalgenerator.h"
#include "ui/diagramview.h"
#include "ui/objectdialog.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <QAction>
#include <QButtonGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QSaveFile>
#include <QFontDatabase>
#include <QFrame>
#include <QGraphicsEllipseItem>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QPolygonF>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QSlider>
#include <QStatusBar>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidgetAction>

using namespace looping;

namespace {

struct VisualNode {
    QRectF bounds;
    bool ellipse = false;
};

class InteractiveLinkItem final : public QGraphicsLineItem {
public:
    InteractiveLinkItem(const QLineF &line, const QPen &pen)
        : QGraphicsLineItem(line), m_visualPen(pen)
    {
        setPen(pen);
        setFlag(QGraphicsItem::ItemIsSelectable);
    }

    QRectF boundingRect() const override
    {
        return QGraphicsLineItem::boundingRect().adjusted(-7, -7, 7, 7);
    }

    QPainterPath shape() const override
    {
        QPainterPath centerLine;
        centerLine.moveTo(line().p1());
        centerLine.lineTo(line().p2());
        QPainterPathStroker stroker;
        stroker.setWidth(14);
        stroker.setCapStyle(Qt::RoundCap);
        return stroker.createStroke(centerLine);
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override
    {
        painter->setRenderHint(QPainter::Antialiasing);
        if (isSelected()) {
            QPen highlight(QColor(37, 99, 235, 90), 7, Qt::SolidLine, Qt::RoundCap);
            painter->setPen(highlight);
            painter->drawLine(line());
        }
        painter->setPen(m_visualPen);
        painter->drawLine(line());
    }

private:
    QPen m_visualPen;
};

QPointF boundaryPoint(const VisualNode &node, const QPointF &towards)
{
    const QPointF center = node.bounds.center();
    const QPointF delta = towards - center;
    if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y())) return center;
    qreal factor;
    if (node.ellipse) {
        const qreal rx = node.bounds.width() / 2.0;
        const qreal ry = node.bounds.height() / 2.0;
        factor = 1.0 / std::sqrt((delta.x() * delta.x()) / (rx * rx) +
                                 (delta.y() * delta.y()) / (ry * ry));
    } else {
        const qreal horizontal = qFuzzyIsNull(delta.x()) ? std::numeric_limits<qreal>::max()
                                                         : node.bounds.width() / (2.0 * qAbs(delta.x()));
        const qreal vertical = qFuzzyIsNull(delta.y()) ? std::numeric_limits<qreal>::max()
                                                       : node.bounds.height() / (2.0 * qAbs(delta.y()));
        factor = qMin(horizontal, vertical);
    }
    return center + delta * factor;
}

void compactNativeLayout(Model &model)
{
    if (model.entities.size() < 2 || model.associations.isEmpty()) return;
    QRectF bounds;
    for (const auto &entity : model.entities) bounds |= QRectF(entity.position, QSizeF(1, 1));
    for (const auto &association : model.associations) bounds |= QRectF(association.position, QSizeF(1, 1));
    if (bounds.width() < 1 || bounds.height() < 1) return;
    const qreal scale = qBound<qreal>(0.45,
        std::min({0.78, 1200.0 / bounds.width(), 820.0 / bounds.height()}), 0.78);
    const QPointF margin(90, 85);
    const auto compact = [&](const QPointF &position) {
        return margin + (position - bounds.topLeft()) * scale;
    };
    for (auto &entity : model.entities) entity.position = compact(entity.position);
    for (auto &association : model.associations) association.position = compact(association.position);
}

QGraphicsDropShadowEffect *softShadow()
{
    auto *effect = new QGraphicsDropShadowEffect;
    effect->setBlurRadius(10);
    effect->setOffset(3, 4);
    effect->setColor(QColor(25, 35, 45, 75));
    return effect;
}

} // namespace

MainWindow::MainWindow() { setupUi(); newModel(); }

static QToolButton *ribbonButton(const QString &symbol, const QString &text, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setText(symbol + "\n" + text);
    button->setToolButtonStyle(Qt::ToolButtonTextOnly);
    button->setMinimumSize(54, 62);
    button->setAutoRaise(true);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

static QFrame *ribbonGroup(const QString &title, QLayout *content, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName("ribbonGroup");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(4, 3, 4, 0);
    layout->setSpacing(0);
    layout->addLayout(content, 1);
    auto *label = new QLabel(title, frame);
    label->setObjectName("ribbonGroupTitle");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return frame;
}

void MainWindow::setupUi()
{
    resize(1120, 720);
    setMinimumSize(900, 580);
    setWindowTitle("OpenLooping");
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
    setStyleSheet(R"(
        QMainWindow { background: #dfe3e8; }
        QWidget { font-family: "DejaVu Sans"; font-size: 10px; }
        #ribbon { background: #fbfcfd; border-bottom: 1px solid #c9d0d7; }
        #ribbonTab { background: white; border: 1px solid #c8d0d8; border-bottom: 0; border-radius: 5px 5px 0 0; padding: 5px 18px; }
        #appButton { color: #1671c5; font-weight: bold; border: 1px solid #b7c3cf; border-radius: 14px; background: white; }
        #appButton:hover { background: #edf6ff; border-color: #78aee0; }
        #ribbonGroup { border-right: 1px solid #d6dce2; background: #fbfcfd; }
        #ribbonGroupTitle { background: #eef2f5; color: #697681; border-top: 1px solid #e1e6ea; min-height: 16px; }
        QToolButton { border: 1px solid transparent; border-radius: 5px; padding: 4px; }
        QToolButton:hover { border: 1px solid #8db8df; background: #e8f3fd; }
        QToolButton:checked { border: 1px solid #4c96d5; background: #d7ebfb; color: #174f7e; }
        QPushButton { border: 1px solid #c4ccd3; border-radius: 4px; padding: 3px 7px; background: #ffffff; }
        QPushButton:hover { border-color: #77aedd; background: #edf6fd; }
        QMdiArea { background: #dce1e6; }
        QMdiSubWindow { background: #f7f8fa; }
        QDockWidget::title { background: #eef1f4; border: 1px solid #c7ced5; padding: 5px; text-align: left; }
        QStatusBar { background: #f5f6f7; border-top: 1px solid #d0d5da; }
    )");

    auto *root = new QWidget;
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *ribbon = new QFrame(root);
    ribbon->setObjectName("ribbon");
    auto *ribbonOuter = new QVBoxLayout(ribbon);
    ribbonOuter->setContentsMargins(5, 2, 0, 0);
    ribbonOuter->setSpacing(0);
    auto *tabs = new QHBoxLayout;
    auto *appButton = new QPushButton("◉", ribbon); appButton->setObjectName("appButton"); appButton->setFixedSize(30, 30);
    auto *tab = new QLabel("Looping", ribbon); tab->setObjectName("ribbonTab");
    auto *style = new QLabel(tr("Style  ▾    ⓘ"), ribbon);
    tabs->addWidget(appButton); tabs->addWidget(tab); tabs->addStretch(); tabs->addWidget(style); tabs->addSpacing(8);
    ribbonOuter->addLayout(tabs);
    auto *groups = new QHBoxLayout; groups->setContentsMargins(0, 0, 0, 0); groups->setSpacing(0);

    auto *formal = new QHBoxLayout;
    auto *ea = ribbonButton("▦", "E/A", ribbon); ea->setCheckable(true); ea->setChecked(true);
    auto *uml = ribbonButton("▤", "UML", ribbon); uml->setEnabled(false);
    auto *mldButton = ribbonButton("▥", "MLD", ribbon);
    formal->addWidget(ea); formal->addWidget(uml); formal->addWidget(mldButton);
    groups->addWidget(ribbonGroup(tr("Formalisme"), formal, ribbon));

    auto *display = new QGridLayout; display->setHorizontalSpacing(8); display->setVerticalSpacing(2);
    auto *rubriques = new QCheckBox(tr("Rubriques")); rubriques->setChecked(true);
    auto *types = new QCheckBox(tr("Types données"));
    auto *illustrations = new QCheckBox(tr("Illustrations")); illustrations->setChecked(true);
    m_zoomLabel = new QLabel(tr("Zoom    100%")); QFont zf = m_zoomLabel->font(); zf.setBold(true); m_zoomLabel->setFont(zf);
    auto *fit = new QPushButton(tr("▣ Ajuster à la fenêtre")); fit->setFlat(true); fit->setCursor(Qt::PointingHandCursor);
    m_zoomSlider = new QSlider(Qt::Horizontal); m_zoomSlider->setRange(25, 200); m_zoomSlider->setValue(100); m_zoomSlider->setFixedWidth(105);
    display->addWidget(rubriques, 0, 0); display->addWidget(m_zoomLabel, 0, 1);
    display->addWidget(types, 1, 0); display->addWidget(fit, 1, 1);
    display->addWidget(illustrations, 2, 0); display->addWidget(m_zoomSlider, 2, 1);
    groups->addWidget(ribbonGroup(tr("Affichage"), display, ribbon));

    auto *pointer = new QHBoxLayout;
    auto *selection = ribbonButton("⌁", tr("Sélection"), ribbon); selection->setCheckable(true); selection->setChecked(true);
    auto *prof = ribbonButton("♟", tr("Prof"), ribbon);
    pointer->addWidget(selection); pointer->addWidget(prof);
    groups->addWidget(ribbonGroup(tr("Pointeur"), pointer, ribbon));

    auto *tools = new QHBoxLayout;
    auto *mcdMenu = ribbonButton("▰", "MCD\n▾", ribbon);
    auto *illustration = ribbonButton("◉", tr("Illustration\n▾"), ribbon);
    auto *edition = ribbonButton("▣", tr("Edition\n▾"), ribbon);
    auto *windows = ribbonButton("▤", tr("Fenêtres\n▾"), ribbon);
    tools->addWidget(mcdMenu); tools->addWidget(illustration); tools->addWidget(edition); tools->addWidget(windows);
    groups->addWidget(ribbonGroup(QString(), tools, ribbon));
    groups->addStretch(); ribbonOuter->addLayout(groups);
    rootLayout->addWidget(ribbon);

    m_mdiArea = new QMdiArea(root);
    m_mdiArea->setViewMode(QMdiArea::SubWindowView);
    m_mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation, true);
    m_scene = new QGraphicsScene(this); m_scene->setSceneRect(0, 0, 2000, 1200);
    m_view = new DiagramView; m_view->setScene(m_scene); m_view->setBackgroundBrush(QColor("#f1f1f1"));
    m_documentWindow = m_mdiArea->addSubWindow(m_view);
    m_documentWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    m_documentWindow->setWindowIcon(windowIcon());
    m_documentWindow->showMaximized();
    rootLayout->addWidget(m_mdiArea, 1);
    setCentralWidget(root);

    auto *fileMenu = new QMenu(this);
    auto *newAction = fileMenu->addAction(tr("Nouveau"), this, &MainWindow::newModel); newAction->setShortcut(QKeySequence::New); addAction(newAction);
    auto *openAction = fileMenu->addAction(tr("Ouvrir..."), this, &MainWindow::openModel); openAction->setShortcut(QKeySequence::Open); addAction(openAction);
    auto *saveAction = fileMenu->addAction(tr("Enregistrer"), this, &MainWindow::saveModel); saveAction->setShortcut(QKeySequence::Save); addAction(saveAction);
    auto *saveAsAction = fileMenu->addAction(tr("Enregistrer sous..."), this, &MainWindow::saveModelAs); saveAsAction->setShortcut(QKeySequence::SaveAs); addAction(saveAsAction);
    auto *exportMenu = fileMenu->addMenu(tr("Exporter"));
    exportMenu->addAction(tr("Script SQL..."), this, &MainWindow::exportSql);
    exportMenu->addAction(tr("MLD textuel..."), this, &MainWindow::exportMld);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Quitter"), this, &QWidget::close);
    appButton->setMenu(fileMenu);

    auto *mcdTools = new QMenu(this);
    auto *paletteAction = new QWidgetAction(mcdTools);
    auto *palette = new QWidget;
    auto *paletteLayout = new QHBoxLayout(palette); paletteLayout->setContentsMargins(5, 5, 5, 5); paletteLayout->setSpacing(2);
    auto addPaletteButton = [&](const QString &symbol, const QString &label, auto callback, bool enabled = true) {
        auto *button = ribbonButton(symbol, label, palette); button->setMinimumSize(62, 54); button->setEnabled(enabled);
        if (enabled) connect(button, &QToolButton::clicked, this, [this, callback, mcdTools] {
            (this->*callback)();
            mcdTools->close();
        });
        paletteLayout->addWidget(button);
    };
    addPaletteButton("▰", tr("Entité"), &MainWindow::addEntity);
    addPaletteButton("⬭", tr("Association"), &MainWindow::addAssociation);
    addPaletteButton("◇", tr("Héritage"), &MainWindow::addInheritance);
    addPaletteButton("⊗", tr("Contrainte"), &MainWindow::addEntity, false);
    addPaletteButton("●", tr("CIF"), &MainWindow::addEntity, false);
    addPaletteButton("▣", tr("Règle"), &MainWindow::addRule);
    addPaletteButton("0,n", tr("Lien"), &MainWindow::startLink);
    addPaletteButton("⌃", tr("Cassure"), &MainWindow::addEntity, false);
    addPaletteButton("▦", tr("Sous-modèle"), &MainWindow::addEntity, false);
    paletteAction->setDefaultWidget(palette); mcdTools->addAction(paletteAction);
    mcdMenu->setMenu(mcdTools); mcdMenu->setPopupMode(QToolButton::InstantPopup);
    connect(mldButton, &QToolButton::clicked, this, [this] { m_mldDock->show(); });
    connect(selection, &QToolButton::clicked, this, &MainWindow::cancelCurrentTool);
    connect(rubriques, &QCheckBox::toggled, this, [this](bool visible) { m_showFields = visible; rebuildScene(); });
    connect(types, &QCheckBox::toggled, this, [this](bool visible) { m_showTypes = visible; rebuildScene(); });
    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        m_view->resetTransform(); m_view->scale(value / 100.0, value / 100.0);
        m_zoomLabel->setText(tr("Zoom    %1%").arg(value));
    });
    connect(m_view, &DiagramView::zoomSteps, this, [this](int steps) { m_zoomSlider->setValue(m_zoomSlider->value() + steps * 10); });
    connect(fit, &QPushButton::clicked, this, &MainWindow::fitModelToView);

    auto makeDock = [this](const QString &title, QPlainTextEdit *&edit, Qt::DockWidgetArea area) {
        auto *dock = new QDockWidget(title, this); edit = new QPlainTextEdit; edit->setReadOnly(true);
        edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont)); dock->setWidget(edit); addDockWidget(area, dock); dock->hide(); return dock;
    };
    m_mldDock = makeDock("MLD", m_mld, Qt::BottomDockWidgetArea);
    m_sqlDock = makeDock("LDD", m_sql, Qt::RightDockWidgetArea);
    auto *windowsMenu = new QMenu(this);
    windowsMenu->addAction(tr("Afficher le modèle"), this, [this] { m_documentWindow->showMaximized(); m_mdiArea->setActiveSubWindow(m_documentWindow); });
    windowsMenu->addSeparator();
    windowsMenu->addAction(tr("Cascade"), m_mdiArea, &QMdiArea::cascadeSubWindows);
    windowsMenu->addAction(tr("Mosaïque"), m_mdiArea, &QMdiArea::tileSubWindows);
    windowsMenu->addSeparator();
    windowsMenu->addAction(m_mldDock->toggleViewAction()); windowsMenu->addAction(m_sqlDock->toggleViewAction());
    windows->setMenu(windowsMenu); windows->setPopupMode(QToolButton::InstantPopup);
    auto *editionMenu = new QMenu(this);
    auto *undoAction = editionMenu->addAction(tr("Annuler"), this, &MainWindow::undo); undoAction->setShortcut(QKeySequence::Undo); addAction(undoAction);
    auto *redoAction = editionMenu->addAction(tr("Rétablir"), this, &MainWindow::redo); redoAction->setShortcut(QKeySequence::Redo); addAction(redoAction);
    editionMenu->addSeparator();
    auto *cutAction = editionMenu->addAction(tr("Couper"), this, &MainWindow::cutSelected); cutAction->setShortcut(QKeySequence::Cut); addAction(cutAction);
    auto *copyAction = editionMenu->addAction(tr("Copier"), this, &MainWindow::copySelected); copyAction->setShortcut(QKeySequence::Copy); addAction(copyAction);
    auto *pasteAction = editionMenu->addAction(tr("Coller"), this, &MainWindow::pasteClipboard); pasteAction->setShortcut(QKeySequence::Paste); addAction(pasteAction);
    auto *duplicateAction = editionMenu->addAction(tr("Dupliquer"), this, &MainWindow::duplicateSelected); duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D)); addAction(duplicateAction);
    editionMenu->addSeparator();
    auto *selectAllAction = editionMenu->addAction(tr("Sélectionner tout"), this, &MainWindow::selectAllObjects); selectAllAction->setShortcut(QKeySequence::SelectAll); addAction(selectAllAction);
    auto *deleteAction = editionMenu->addAction(tr("Supprimer"), this, &MainWindow::deleteSelected); deleteAction->setShortcut(QKeySequence::Delete); addAction(deleteAction);
    edition->setMenu(editionMenu); edition->setPopupMode(QToolButton::InstantPopup);
    connect(m_view, &DiagramView::objectClicked, this, &MainWindow::objectClicked);
    connect(m_view, &DiagramView::objectDoubleClicked, this, &MainWindow::editObject);
    connect(m_view, &DiagramView::emptyClicked, this, &MainWindow::placeObject);
    connect(m_view, &DiagramView::objectMoved, this, &MainWindow::moveObject);
    connect(m_view, &DiagramView::contextRequested, this, &MainWindow::showObjectMenu);
    connect(m_view, &DiagramView::deleteRequested, this, &MainWindow::deleteSelected);
    connect(m_view, &DiagramView::cancelRequested, this, &MainWindow::cancelCurrentTool);
}

void MainWindow::cancelCurrentTool()
{
    m_pendingPlacement.clear();
    m_linkMode = false;
    m_linkStartKind.clear();
    m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->clearMessage();
}

void MainWindow::newModel()
{
    if (!confirmDiscard()) return;
    m_model = {}; m_path.clear(); m_dirty = false; m_linkMode = false;
    m_linkStartKind.clear(); m_linkStartId.clear(); m_pendingPlacement.clear();
    m_view->setConnectionMode(false);
    m_undo.clear(); m_redo.clear(); m_documentWindow->showMaximized(); rebuildScene();
}

void MainWindow::openModel()
{
    if (!confirmDiscard()) return;
    const auto path = QFileDialog::getOpenFileName(
        this, tr("Ouvrir"), {},
        tr("Tous les modèles Looping (*.lrea *.loo *.lo1 *.lo2 *.lo3 *.lo4 *.lo5 *.lo6 *.lo7 *.lo8 *.lo9);;"
           "Archives Looping Windows (*.loo *.lo1 *.lo2 *.lo3 *.lo4 *.lo5 *.lo6 *.lo7 *.lo8 *.lo9);;"
           "Modèles OpenLooping (*.lrea *.json);;Tous les fichiers (*)"));
    if (path.isEmpty()) return;
    loadModel(path);
}

bool MainWindow::loadModel(const QString &path, bool showWarnings)
{
    QString error; QStringList warnings; Model loaded;
    const bool native = Persistence::isNativeLoopingFile(path);
    if (!Persistence::load(loaded, path, &error, &warnings)) {
        QMessageBox::critical(this, tr("Erreur d'import"), error);
        return false;
    }
    if (native) compactNativeLayout(loaded);
    m_model = std::move(loaded);
    m_path = native ? QString() : path;
    m_dirty = native; m_undo.clear(); m_redo.clear();
    m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear(); m_pendingPlacement.clear();
    m_view->setConnectionMode(false);
    m_documentWindow->showMaximized(); rebuildScene();
    QTimer::singleShot(0, this, &MainWindow::fitModelToView);
    if (showWarnings && !warnings.isEmpty()) QMessageBox::information(this, tr("Import Looping"), warnings.join("\n\n"));
    return true;
}

bool MainWindow::saveModel() { return m_path.isEmpty() ? saveModelAs() : writeModel(m_path); }
bool MainWindow::saveModelAs()
{
    const auto path = QFileDialog::getSaveFileName(this, tr("Enregistrer"), "modele.lrea", tr("Modèles OpenLooping (*.lrea)"));
    return path.isEmpty() ? false : writeModel(path);
}

bool MainWindow::writeModel(const QString &path)
{
    QString error;
    if (!Persistence::save(m_model, path, &error)) { QMessageBox::critical(this, tr("Erreur"), error); return false; }
    m_path = path; setDirty(false); return true;
}

static bool writeTextFile(QWidget *parent, const QString &title, const QString &suggestedName,
                          const QString &filter, const QString &contents)
{
    const QString path = QFileDialog::getSaveFileName(parent, title, suggestedName, filter);
    if (path.isEmpty()) return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { QMessageBox::critical(parent, QObject::tr("Erreur"), file.errorString()); return false; }
    file.write(contents.toUtf8());
    if (!file.commit()) { QMessageBox::critical(parent, QObject::tr("Erreur"), file.errorString()); return false; }
    return true;
}

void MainWindow::exportSql()
{
    writeTextFile(this, tr("Exporter le script SQL"), "modele.sql", tr("Scripts SQL (*.sql)"), RelationalGenerator::generate(m_model).sql + '\n');
}

void MainWindow::exportMld()
{
    writeTextFile(this, tr("Exporter le MLD textuel"), "modele.txt", tr("Fichiers texte (*.txt)"), RelationalGenerator::generate(m_model).mld + '\n');
}

void MainWindow::addEntity()
{
    m_pendingPlacement = "entity"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer l’entité"));
}

void MainWindow::placeObject(const QPointF &position)
{
    if (m_pendingPlacement.isEmpty()) return;
    const QString kind = m_pendingPlacement;
    m_pendingPlacement.clear();
    if (kind == "entity") {
    ObjectDialog dialog(tr("Nouvelle entité"), this);
    const QString name = tr("Entité %1").arg(m_model.entities.size() + 1);
    dialog.setObject(name, normalizedLogicalName(name), {});
    if (dialog.exec() != QDialog::Accepted || dialog.objectName().isEmpty()) return;
    checkpoint();
    m_model.entities.append({newId(), dialog.objectName(), dialog.logicalName(), position, dialog.fields()});
    setDirty(); rebuildScene();
    } else if (kind == "association") {
        ObjectDialog dialog(tr("Nouvelle association"), this);
        const QString name = tr("Asso %1").arg(m_model.associations.size() + 1);
        dialog.setObject(name, normalizedLogicalName(name), {});
        if (dialog.exec() != QDialog::Accepted || dialog.objectName().isEmpty()) return;
        checkpoint();
        m_model.associations.append({newId(), dialog.objectName(), dialog.logicalName(), position, dialog.fields()});
        setDirty(); rebuildScene();
    } else if (kind == "inheritance") {
        Inheritance inheritance{newId(), tr("Héritage %1").arg(m_model.inheritances.size() + 1), position};
        InheritanceDialog dialog(inheritance, this);
        if (dialog.exec() != QDialog::Accepted) return;
        checkpoint(); m_model.inheritances.append(dialog.inheritance()); setDirty(); rebuildScene();
    } else if (kind == "rule") {
        Rule rule{newId(), tr("Règle %1").arg(m_model.rules.size() + 1), position};
        RuleDialog dialog(rule, this);
        if (dialog.exec() != QDialog::Accepted || dialog.rule().name.isEmpty()) return;
        checkpoint(); m_model.rules.append(dialog.rule()); setDirty(); rebuildScene();
    }
    statusBar()->clearMessage();
}

void MainWindow::addAssociation()
{
    m_pendingPlacement = "association"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer l’association"));
}

void MainWindow::addInheritance()
{
    m_pendingPlacement = "inheritance"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer l’héritage"));
}

void MainWindow::addRule()
{
    m_pendingPlacement = "rule"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer la règle"));
}

void MainWindow::startLink()
{
    m_pendingPlacement.clear();
    m_linkMode = true;
    m_linkStartKind.clear();
    m_linkStartId.clear();
    m_view->setConnectionMode(true);
    statusBar()->showMessage(tr("Mode lien : cliquez une entité, une association ou un héritage"));
}

void MainWindow::moveObject(const QString &kind, const QString &id, const QPointF &position)
{
    if (kind == "entity") { const auto *object = m_model.entity(id); if (!object || object->position == position) return; }
    else if (kind == "association") { const auto *object = m_model.association(id); if (!object || object->position == position) return; }
    else if (kind == "inheritance") { const auto *object = m_model.inheritance(id); if (!object || object->position == position) return; }
    else if (kind == "rule") { const auto *object = m_model.rule(id); if (!object || object->position == position) return; }
    else return;
    checkpoint();
    if (kind == "entity") { if (auto *object = m_model.entity(id)) object->position = position; }
    else if (kind == "association") { if (auto *object = m_model.association(id)) object->position = position; }
    else if (kind == "inheritance") { if (auto *object = m_model.inheritance(id)) object->position = position; }
    else if (kind == "rule") { if (auto *object = m_model.rule(id)) object->position = position; }
    setDirty(); rebuildScene();
}

void MainWindow::showObjectMenu(const QString &kind, const QString &id, const QPoint &globalPosition)
{
    QMenu menu(this);
    menu.addAction(tr("Propriétés"), this, [this, kind, id] { editObject(kind, id); });
    menu.addAction(tr("Supprimer"), this, [this, kind, id] {
        for (auto *item : m_scene->items()) if (item->data(0).toString() == kind && item->data(1).toString() == id) item->setSelected(true);
        deleteSelected();
    });
    menu.exec(globalPosition);
}

void MainWindow::copySelected()
{
    QStringList entityIds, associationIds, linkIds, inheritanceIds, inheritanceLinkIds, ruleIds;
    for (auto *item : m_scene->selectedItems()) {
        while (item->parentItem()) item = item->parentItem();
        const QString kind = item->data(0).toString(), id = item->data(1).toString();
        if (kind == "entity" && !entityIds.contains(id)) entityIds << id;
        else if (kind == "association" && !associationIds.contains(id)) associationIds << id;
        else if (kind == "link" && !linkIds.contains(id)) linkIds << id;
        else if (kind == "inheritance" && !inheritanceIds.contains(id)) inheritanceIds << id;
        else if (kind == "inheritance-link" && !inheritanceLinkIds.contains(id)) inheritanceLinkIds << id;
        else if (kind == "rule" && !ruleIds.contains(id)) ruleIds << id;
    }
    Model copied;
    for (const auto &entity : m_model.entities) if (entityIds.contains(entity.id)) copied.entities.append(entity);
    for (const auto &association : m_model.associations) if (associationIds.contains(association.id)) copied.associations.append(association);
    for (const auto &link : m_model.links)
        if (linkIds.contains(link.id) || (entityIds.contains(link.entityId) && associationIds.contains(link.associationId))) copied.links.append(link);
    for (const auto &inheritance : m_model.inheritances) if (inheritanceIds.contains(inheritance.id)) copied.inheritances.append(inheritance);
    for (const auto &link : m_model.inheritanceLinks)
        if (inheritanceLinkIds.contains(link.id) || (entityIds.contains(link.entityId) && inheritanceIds.contains(link.inheritanceId))) copied.inheritanceLinks.append(link);
    for (const auto &rule : m_model.rules) if (ruleIds.contains(rule.id)) copied.rules.append(rule);
    m_clipboard = std::move(copied);
    m_pasteGeneration = 0;
    statusBar()->showMessage(tr("%1 objet(s) copié(s)").arg(m_clipboard.entities.size() + m_clipboard.associations.size() + m_clipboard.inheritances.size() + m_clipboard.rules.size()), 2000);
}

void MainWindow::cutSelected()
{
    copySelected();
    if (!m_clipboard.entities.isEmpty() || !m_clipboard.associations.isEmpty() || !m_clipboard.inheritances.isEmpty() || !m_clipboard.rules.isEmpty()) deleteSelected();
}

void MainWindow::pasteClipboard()
{
    if (m_clipboard.entities.isEmpty() && m_clipboard.associations.isEmpty() && m_clipboard.inheritances.isEmpty() && m_clipboard.rules.isEmpty()) return;
    checkpoint();
    ++m_pasteGeneration;
    const QPointF offset(25 * m_pasteGeneration, 25 * m_pasteGeneration);
    QHash<QString, QString> ids;
    QStringList pastedIds;
    for (auto entity : m_clipboard.entities) {
        const QString previous = entity.id; entity.id = newId(); entity.position += offset;
        for (auto &field : entity.fields) field.id = newId();
        ids.insert(previous, entity.id); pastedIds << entity.id; m_model.entities.append(entity);
    }
    for (auto association : m_clipboard.associations) {
        const QString previous = association.id; association.id = newId(); association.position += offset;
        for (auto &field : association.fields) field.id = newId();
        ids.insert(previous, association.id); pastedIds << association.id; m_model.associations.append(association);
    }
    for (auto inheritance : m_clipboard.inheritances) {
        const QString previous = inheritance.id; inheritance.id = newId(); inheritance.position += offset;
        ids.insert(previous, inheritance.id); pastedIds << inheritance.id; m_model.inheritances.append(inheritance);
    }
    for (auto rule : m_clipboard.rules) {
        rule.id = newId(); rule.position += offset; pastedIds << rule.id; m_model.rules.append(rule);
    }
    for (auto link : m_clipboard.links) {
        if (!ids.contains(link.entityId) || !ids.contains(link.associationId)) continue;
        link.id = newId(); link.entityId = ids.value(link.entityId); link.associationId = ids.value(link.associationId); m_model.links.append(link);
    }
    for (auto link : m_clipboard.inheritanceLinks) {
        if (!ids.contains(link.entityId) || !ids.contains(link.inheritanceId)) continue;
        link.id = newId(); link.entityId = ids.value(link.entityId); link.inheritanceId = ids.value(link.inheritanceId); m_model.inheritanceLinks.append(link);
    }
    setDirty(); rebuildScene();
    for (auto *item : m_scene->items()) {
        while (item->parentItem()) item = item->parentItem();
        if (pastedIds.contains(item->data(1).toString())) item->setSelected(true);
    }
}

void MainWindow::duplicateSelected()
{
    copySelected();
    pasteClipboard();
}

void MainWindow::selectAllObjects()
{
    for (auto *item : m_scene->items())
        if (!item->parentItem() && !item->data(0).toString().isEmpty()) item->setSelected(true);
}

void MainWindow::deleteSelected()
{
    QStringList entityIds, associationIds, linkIds, inheritanceIds, inheritanceLinkIds, ruleIds;
    for (auto *item : m_scene->selectedItems()) {
        while (item->parentItem()) item = item->parentItem();
        const QString kind = item->data(0).toString(), id = item->data(1).toString();
        if (kind == "entity") entityIds << id;
        else if (kind == "association") associationIds << id;
        else if (kind == "link") linkIds << id;
        else if (kind == "inheritance") inheritanceIds << id;
        else if (kind == "inheritance-link") inheritanceLinkIds << id;
        else if (kind == "rule") ruleIds << id;
    }
    if (entityIds.isEmpty() && associationIds.isEmpty() && linkIds.isEmpty() && inheritanceIds.isEmpty() && inheritanceLinkIds.isEmpty() && ruleIds.isEmpty()) return;
    checkpoint();
    m_model.links.erase(std::remove_if(m_model.links.begin(), m_model.links.end(), [&](const Link &link) {
        return linkIds.contains(link.id) || entityIds.contains(link.entityId) || associationIds.contains(link.associationId);
    }), m_model.links.end());
    m_model.inheritanceLinks.erase(std::remove_if(m_model.inheritanceLinks.begin(), m_model.inheritanceLinks.end(), [&](const InheritanceLink &link) {
        return inheritanceLinkIds.contains(link.id) || entityIds.contains(link.entityId) || inheritanceIds.contains(link.inheritanceId);
    }), m_model.inheritanceLinks.end());
    m_model.entities.erase(std::remove_if(m_model.entities.begin(), m_model.entities.end(), [&](const Entity &entity) { return entityIds.contains(entity.id); }), m_model.entities.end());
    m_model.associations.erase(std::remove_if(m_model.associations.begin(), m_model.associations.end(), [&](const Association &association) { return associationIds.contains(association.id); }), m_model.associations.end());
    m_model.inheritances.erase(std::remove_if(m_model.inheritances.begin(), m_model.inheritances.end(), [&](const Inheritance &inheritance) { return inheritanceIds.contains(inheritance.id); }), m_model.inheritances.end());
    m_model.rules.erase(std::remove_if(m_model.rules.begin(), m_model.rules.end(), [&](const Rule &rule) { return ruleIds.contains(rule.id); }), m_model.rules.end());
    setDirty(); rebuildScene();
}

void MainWindow::objectClicked(const QString &kind, const QString &id)
{
    if (!m_linkMode) return;

    const bool supported = kind == "entity" || kind == "association" || kind == "inheritance";
    if (!supported || id.isEmpty()) {
        statusBar()->showMessage(tr("Un lien relie une entité à une association ou à un héritage"), 3000);
        return;
    }

    if (m_linkStartId.isEmpty()) {
        m_linkStartKind = kind;
        m_linkStartId = id;
        m_view->setConnectionStart(kind, id);
        if (kind == "entity")
            statusBar()->showMessage(tr("Entité sélectionnée : cliquez une association ou un héritage"));
        else
            statusBar()->showMessage(tr("Extrémité sélectionnée : cliquez une entité"));
        return;
    }

    if (m_linkStartKind == kind && m_linkStartId == id) {
        m_linkStartKind.clear();
        m_linkStartId.clear();
        m_view->clearConnectionStart();
        statusBar()->showMessage(tr("Sélection annulée : choisissez une première extrémité"));
        return;
    }

    QString entityId;
    QString associationId;
    QString inheritanceId;
    if (m_linkStartKind == "entity" && kind == "association") {
        entityId = m_linkStartId;
        associationId = id;
    } else if (m_linkStartKind == "association" && kind == "entity") {
        entityId = id;
        associationId = m_linkStartId;
    } else if (m_linkStartKind == "entity" && kind == "inheritance") {
        entityId = m_linkStartId;
        inheritanceId = id;
    } else if (m_linkStartKind == "inheritance" && kind == "entity") {
        entityId = id;
        inheritanceId = m_linkStartId;
    } else {
        m_linkStartKind = kind;
        m_linkStartId = id;
        m_view->setConnectionStart(kind, id);
        statusBar()->showMessage(kind == "entity"
            ? tr("Entité sélectionnée : cliquez une association ou un héritage")
            : tr("Extrémité sélectionnée : cliquez une entité"));
        return;
    }

    if (!associationId.isEmpty()) {
        for (const auto &link : m_model.links)
            if (link.entityId == entityId && link.associationId == associationId) {
                m_linkStartKind.clear(); m_linkStartId.clear();
                m_view->clearConnectionStart();
                statusBar()->showMessage(tr("Ce lien existe déjà — choisissez une autre extrémité"), 3000);
                return;
            }
        checkpoint();
        m_model.links.append({newId(), entityId, associationId, Cardinality::ZeroMany, {}, {}, {}, 0});
        m_linkStartKind.clear(); m_linkStartId.clear(); m_view->clearConnectionStart(); setDirty(); rebuildScene();
        statusBar()->showMessage(tr("Lien 0,n créé — cliquez pour en ajouter un autre"), 3000);
    } else if (!inheritanceId.isEmpty()) {
        for (const auto &link : m_model.inheritanceLinks)
            if (link.entityId == entityId && link.inheritanceId == inheritanceId) {
                m_linkStartKind.clear(); m_linkStartId.clear();
                m_view->clearConnectionStart();
                statusBar()->showMessage(tr("Ce lien d’héritage existe déjà — choisissez une autre extrémité"), 3000);
                return;
            }
        bool hasSupertype = false;
        for (const auto &link : m_model.inheritanceLinks)
            if (link.inheritanceId == inheritanceId && link.supertype) hasSupertype = true;
        checkpoint();
        m_model.inheritanceLinks.append({newId(), entityId, inheritanceId, !hasSupertype});
        m_linkStartKind.clear(); m_linkStartId.clear(); m_view->clearConnectionStart(); setDirty(); rebuildScene();
        statusBar()->showMessage(hasSupertype
            ? tr("Lien de sous-type créé — cliquez pour en ajouter un autre")
            : tr("Lien de surtype créé — cliquez pour en ajouter un autre"), 3000);
    }
}

void MainWindow::editObject(const QString &kind, const QString &id)
{
    if (kind == "entity") {
        auto *e = m_model.entity(id); if (!e) return;
        ObjectDialog d(tr("Propriétés de l’entité"), this); d.setObject(e->name, e->logicalName, e->fields);
        if (d.exec() == QDialog::Accepted) { checkpoint(); e->name = d.objectName(); e->logicalName = d.logicalName(); e->fields = d.fields(); setDirty(); rebuildScene(); }
    } else if (kind == "association") {
        auto *a = m_model.association(id); if (!a) return;
        ObjectDialog d(tr("Propriétés de l’association"), this); d.setObject(a->name, a->logicalName, a->fields);
        if (d.exec() == QDialog::Accepted) { checkpoint(); a->name = d.objectName(); a->logicalName = d.logicalName(); a->fields = d.fields(); setDirty(); rebuildScene(); }
    } else if (kind == "link") {
        auto *l = m_model.link(id); if (!l) return; LinkDialog d(*l, this);
        if (d.exec() == QDialog::Accepted) { const auto changed = d.link(); checkpoint(); *l = changed; setDirty(); rebuildScene(); }
    } else if (kind == "inheritance") {
        auto *h = m_model.inheritance(id); if (!h) return; InheritanceDialog d(*h, this);
        if (d.exec() == QDialog::Accepted) { const auto changed = d.inheritance(); checkpoint(); *h = changed; setDirty(); rebuildScene(); }
    } else if (kind == "inheritance-link") {
        auto *l = m_model.inheritanceLink(id); if (!l) return; InheritanceLinkDialog d(*l, this);
        if (d.exec() == QDialog::Accepted) { const auto changed = d.link(); checkpoint(); *l = changed; setDirty(); rebuildScene(); }
    } else if (kind == "rule") {
        auto *r = m_model.rule(id); if (!r) return; RuleDialog d(*r, this);
        if (d.exec() == QDialog::Accepted) { const auto changed = d.rule(); checkpoint(); *r = changed; setDirty(); rebuildScene(); }
    }
}

void MainWindow::rebuildScene()
{
    m_scene->clear();
    QHash<QString, VisualNode> nodes;
    for (const auto &e : m_model.entities) {
        const QFont titleFont("DejaVu Sans", 10, QFont::Bold);
        const QFont fieldFont("DejaVu Sans", 9);
        const QFontMetrics titleMetrics(titleFont);
        qreal width = qMax<qreal>(128, titleMetrics.horizontalAdvance(e.name) + 30);
        if (m_showFields) for (const auto &field : e.fields) {
            const QString label = field.name + (m_showTypes ? QStringLiteral(" : %1").arg(dataTypeName(field)) : QString());
            QFont measuredFont = fieldFont; measuredFont.setBold(field.primaryKey);
            width = qMax<qreal>(width, QFontMetrics(measuredFont).horizontalAdvance(label) + 28);
        }
        width = qMin<qreal>(420, width);
        const qreal headerHeight = 34;
        const qreal bodyHeight = qMax<qreal>(32, (m_showFields ? e.fields.size() : 0) * 24 + 8);
        const qreal height = headerHeight + bodyHeight;
        QPainterPath entityPath; entityPath.addRoundedRect(QRectF(QPointF(), QSizeF(width, height)), 7, 7);
        auto *box = m_scene->addPath(entityPath, QPen(QColor("#65572f"), 1.2), QBrush(QColor("#fff8bd")));
        box->setPos(e.position);
        box->setData(0, "entity"); box->setData(1, e.id);
        box->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        box->setFlag(QGraphicsItem::ItemClipsChildrenToShape);
        box->setGraphicsEffect(softShadow());
        auto *header = new QGraphicsRectItem(QRectF(0, 0, width, headerHeight), box); header->setPen(Qt::NoPen); header->setBrush(QColor("#f2c95f"));
        auto *separator = new QGraphicsLineItem(QLineF(0, headerHeight, width, headerHeight), box);
        separator->setPen(QPen(QColor("#8b773e"), 1));
        auto *title = new QGraphicsTextItem(e.name, box); title->setDefaultTextColor(QColor("#2d2a20")); title->setFont(titleFont);
        title->setPos((width - title->boundingRect().width()) / 2.0, 3);
        int y = headerHeight + 2;
        if (m_showFields) for (const auto &field : e.fields) {
            const QString label = field.name + (m_showTypes ? QStringLiteral(" : %1").arg(dataTypeName(field)) : QString());
            QFont font = fieldFont; font.setUnderline(field.primaryKey); font.setBold(field.primaryKey);
            const QString visibleLabel = QFontMetrics(font).elidedText(label, Qt::ElideRight, qRound(width - 22));
            auto *text = new QGraphicsTextItem(visibleLabel, box); text->setDefaultTextColor(QColor("#302f27")); text->setPos(10, y);
            text->setFont(font);
            QStringList properties;
            if (field.primaryKey) properties << tr("Identifiant");
            if (field.notNull) properties << QStringLiteral("NOT NULL");
            if (field.unique) properties << QStringLiteral("UNIQUE");
            text->setToolTip(properties.isEmpty() ? label : label + QStringLiteral("\n") + properties.join(QStringLiteral(" · ")));
            y += 24;
        }
        nodes[e.id] = {QRectF(e.position, QSizeF(width, height)), false};
    }
    for (const auto &a : m_model.associations) {
        qreal width = qMax<qreal>(116, QFontMetrics(QFont("DejaVu Sans", 10, QFont::Bold)).horizontalAdvance(a.name) + 34);
        if (m_showFields) for (const auto &field : a.fields) {
            const QString label = field.name + (m_showTypes ? QStringLiteral(" : %1").arg(dataTypeName(field)) : QString());
            width = qMax<qreal>(width, QFontMetrics(QFont("DejaVu Sans", 9)).horizontalAdvance(label) + 30);
        }
        const qreal height = qMax<qreal>(64, 40 + (m_showFields ? a.fields.size() : 0) * 20);
        const QSizeF size(width, height);
        auto *ellipse = m_scene->addEllipse(QRectF(QPointF(), size), QPen(QColor("#286f76"), 1.3), QBrush(QColor("#61d5da")));
        ellipse->setPos(a.position);
        ellipse->setData(0, "association"); ellipse->setData(1, a.id);
        ellipse->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        ellipse->setGraphicsEffect(softShadow());
        auto *separator = new QGraphicsLineItem(QLineF(8, 31, width - 8, 31), ellipse); separator->setPen(QPen(QColor("#318d93"), 1));
        auto *title = new QGraphicsTextItem(a.name, ellipse); QFont font("DejaVu Sans", 10); font.setBold(true); title->setFont(font); title->setDefaultTextColor(QColor("#163b3e")); title->setPos((width - title->boundingRect().width()) / 2, 4);
        int y = 29;
        if (m_showFields) for (const auto &field : a.fields) {
            const QString label = field.name + (m_showTypes ? QStringLiteral(" : %1").arg(dataTypeName(field)) : QString());
            auto *text = new QGraphicsTextItem(label, ellipse); text->setFont(QFont("DejaVu Sans", 8)); text->setPos((width - text->boundingRect().width()) / 2, y); y += 19;
        }
        nodes[a.id] = {QRectF(a.position, size), true};
    }
    for (const auto &h : m_model.inheritances) {
        const QSizeF size(64, 44);
        QPolygonF diamond; diamond << QPointF(size.width() / 2, 0) << QPointF(size.width(), size.height() / 2)
                                  << QPointF(size.width() / 2, size.height()) << QPointF(0, size.height() / 2);
        auto *shape = m_scene->addPolygon(diamond, QPen(Qt::black), QBrush(QColor("#f7f7f7")));
        shape->setPos(h.position); shape->setData(0, "inheritance"); shape->setData(1, h.id);
        shape->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        const QString marker = inheritanceConstraintName(h.constraint);
        auto *label = new QGraphicsTextItem(marker.isEmpty() ? h.name : marker, shape);
        QFont font("DejaVu Sans", 9); font.setBold(true); label->setFont(font);
        label->setPos((size.width() - label->boundingRect().width()) / 2, (size.height() - label->boundingRect().height()) / 2 - 2);
        nodes[h.id] = {QRectF(h.position, size), false};
    }
    for (const auto &rule : m_model.rules) {
        QStringList body;
        if (rule.showDescription && !rule.description.trimmed().isEmpty()) body << rule.description.trimmed();
        if (rule.showSqlInModel && !rule.sqlScript.trimmed().isEmpty()) body << rule.sqlScript.trimmed();
        const qreal width = 230;
        auto *content = new QGraphicsTextItem(body.join("\n\n")); content->setTextWidth(width - 16); content->setFont(QFont("DejaVu Sans", 8));
        const qreal bodyHeight = body.isEmpty() ? 10 : content->boundingRect().height() + 8;
        const qreal height = 31 + bodyHeight;
        auto *box = m_scene->addRect(QRectF(QPointF(), QSizeF(width, height)), QPen(QColor("#555")), QBrush(QColor("#fff6c7")));
        box->setPos(rule.position); box->setData(0, "rule"); box->setData(1, rule.id);
        box->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
        auto *header = new QGraphicsRectItem(QRectF(0, 0, width, 31), box); header->setPen(Qt::NoPen); header->setBrush(QColor("#f1d36c"));
        auto *title = new QGraphicsTextItem(rule.name, box); QFont font("DejaVu Sans", 9); font.setBold(true); title->setFont(font); title->setPos(7, 3);
        content->setParentItem(box); content->setPos(7, 31);
    }
    for (const auto &l : m_model.links) {
        if (!nodes.contains(l.entityId) || !nodes.contains(l.associationId)) continue;
        const auto entityNode = nodes.value(l.entityId);
        const auto associationNode = nodes.value(l.associationId);
        QLineF geometry(boundaryPoint(entityNode, associationNode.bounds.center()),
                        boundaryPoint(associationNode, entityNode.bounds.center()));
        QPen linkPen(QColor("#59656d"), 1.35); linkPen.setCapStyle(Qt::RoundCap);
        auto *line = new InteractiveLinkItem(geometry, linkPen);
        m_scene->addItem(line);
        line->setData(0, "link"); line->setData(1, l.id); line->setZValue(-1);
        const QPointF direction = geometry.p2() - geometry.p1();
        const qreal length = geometry.length();
        const QPointF normal = length > 0 ? QPointF(-direction.y() / length * 9, direction.x() / length * 9) : QPointF();
        const QString cardinality = cardinalityName(l);
        const QFont labelFont("DejaVu Sans", 8, QFont::DemiBold);
        const QFontMetrics labelMetrics(labelFont);
        const QString fullLabel = l.role.trimmed().isEmpty()
            ? cardinality
            : QStringLiteral("%1  ·  %2").arg(cardinality, l.role.trimmed());
        const QString displayLabel = labelMetrics.elidedText(fullLabel, Qt::ElideRight, 220);
        const QSizeF labelSize(labelMetrics.horizontalAdvance(displayLabel) + 10, 20);
        auto *badge = new QGraphicsRectItem(QRectF(QPointF(), labelSize), line);
        badge->setPen(QPen(QColor("#d2d7da"), 0.8)); badge->setBrush(QColor(255, 255, 255, 235));
        badge->setPos(geometry.pointAt(0.16) + normal - QPointF(labelSize.width() / 2, labelSize.height() / 2));
        badge->setZValue(2);
        badge->setToolTip(fullLabel);
        auto *label = new QGraphicsTextItem(displayLabel, badge); label->setFont(labelFont); label->setDefaultTextColor(QColor("#344047")); label->setPos(4, -1);
    }
    for (const auto &l : m_model.inheritanceLinks) {
        if (!nodes.contains(l.entityId) || !nodes.contains(l.inheritanceId)) continue;
        QLineF geometry(boundaryPoint(nodes.value(l.entityId), nodes.value(l.inheritanceId).bounds.center()),
                        boundaryPoint(nodes.value(l.inheritanceId), nodes.value(l.entityId).bounds.center()));
        auto *line = m_scene->addLine(geometry, QPen(Qt::black, l.supertype ? 2 : 1));
        line->setData(0, "inheritance-link"); line->setData(1, l.id); line->setZValue(-1); line->setFlag(QGraphicsItem::ItemIsSelectable);
        if (l.display && !l.expression.isEmpty()) {
            auto *label = new QGraphicsTextItem(l.expression, line); label->setFont(QFont("DejaVu Sans", 8)); label->setPos(geometry.pointAt(0.5) + QPointF(4, -18));
        }
    }
    updateGeneratedOutput();
    const QRectF contentBounds = m_scene->itemsBoundingRect().adjusted(-80, -80, 80, 80);
    if (contentBounds.isValid() && !contentBounds.isEmpty())
        m_scene->setSceneRect(contentBounds);
    m_documentWindow->setWindowTitle(QStringLiteral("%1%2").arg(m_dirty ? "*" : "", m_path.isEmpty() ? QStringLiteral("nouveau-modele.lrea") : QFileInfo(m_path).fileName()));
    setWindowTitle(QStringLiteral("%1%2 — OpenLooping").arg(m_dirty ? "*" : "", m_model.name));
}

void MainWindow::fitModelToView()
{
    const QRectF bounds = m_scene->itemsBoundingRect().adjusted(-35, -35, 35, 35);
    if (!bounds.isValid() || bounds.isEmpty()) return;
    m_view->fitInView(bounds, Qt::KeepAspectRatio);
    const int value = qBound(25, qRound(m_view->transform().m11() * 100), 200);
    m_zoomSlider->blockSignals(true);
    m_zoomSlider->setValue(value);
    m_zoomSlider->blockSignals(false);
    m_zoomLabel->setText(tr("Zoom    %1%").arg(value));
}

void MainWindow::updateGeneratedOutput()
{
    const auto output = RelationalGenerator::generate(m_model);
    m_mld->setPlainText(output.mld + (output.warnings.isEmpty() ? QString() : "\n\n⚠ " + output.warnings.join("\n⚠ ")));
    m_sql->setPlainText(output.sql);
}

void MainWindow::setDirty(bool dirty) { m_dirty = dirty; }

void MainWindow::checkpoint()
{
    m_undo.append(m_model);
    if (m_undo.size() > 100) m_undo.removeFirst();
    m_redo.clear();
}

void MainWindow::undo()
{
    if (m_undo.isEmpty()) return;
    m_redo.append(m_model);
    m_model = m_undo.takeLast();
    setDirty(); rebuildScene();
}

void MainWindow::redo()
{
    if (m_redo.isEmpty()) return;
    m_undo.append(m_model);
    m_model = m_redo.takeLast();
    setDirty(); rebuildScene();
}

bool MainWindow::confirmDiscard()
{
    if (!m_dirty) return true;
    const auto answer = QMessageBox::warning(this, tr("Modèle modifié"), tr("Enregistrer les modifications du modèle ?"),
                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                              QMessageBox::Save);
    if (answer == QMessageBox::Cancel) return false;
    if (answer == QMessageBox::Save) return saveModel();
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscard()) event->accept(); else event->ignore();
}
