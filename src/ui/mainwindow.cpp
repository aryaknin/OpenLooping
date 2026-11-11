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
#include <QPainter>
#include <QPushButton>
#include <QPolygonF>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QSlider>
#include <QStatusBar>
#include <QStyle>
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

static QIcon domainIcon(const QString &kind, const QColor &color = QColor("#334155"))
{
    QPixmap pixmap(48, 48);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(QColor(255, 255, 255, 230));
    if (kind == "new") {
        painter.drawRoundedRect(QRectF(13, 7, 23, 34), 2, 2);
        painter.drawLine(QPointF(19, 24), QPointF(30, 24));
        painter.drawLine(QPointF(24.5, 18.5), QPointF(24.5, 29.5));
    } else if (kind == "open") {
        QPainterPath folder;
        folder.moveTo(7, 17); folder.lineTo(7, 12); folder.lineTo(21, 12);
        folder.lineTo(25, 17); folder.lineTo(41, 17); folder.lineTo(37, 38);
        folder.lineTo(10, 38); folder.closeSubpath(); painter.drawPath(folder);
        painter.drawLine(QPointF(10, 21), QPointF(38, 21));
    } else if (kind == "save") {
        painter.drawRoundedRect(QRectF(9, 8, 30, 32), 3, 3);
        painter.drawRect(QRectF(15, 9, 17, 10));
        painter.drawRoundedRect(QRectF(15, 27, 18, 13), 2, 2);
    } else if (kind == "entity") {
        painter.drawRoundedRect(QRectF(8, 9, 32, 30), 4, 4);
        painter.drawLine(QPointF(8, 18), QPointF(40, 18));
        painter.drawLine(QPointF(13, 24), QPointF(31, 24));
        painter.drawLine(QPointF(13, 30), QPointF(35, 30));
    } else if (kind == "association") {
        painter.drawEllipse(QRectF(6, 12, 36, 24));
        painter.drawLine(QPointF(15, 24), QPointF(33, 24));
    } else if (kind == "inheritance") {
        QPolygonF diamond; diamond << QPointF(24, 6) << QPointF(42, 24)
                                  << QPointF(24, 42) << QPointF(6, 24);
        painter.drawPolygon(diamond);
        painter.drawLine(QPointF(18, 24), QPointF(30, 24));
    } else if (kind == "rule") {
        painter.setBrush(QColor("#fff7d6"));
        painter.drawRoundedRect(QRectF(8, 8, 32, 32), 4, 4);
        painter.drawLine(QPointF(13, 17), QPointF(34, 17));
        painter.drawLine(QPointF(13, 24), QPointF(31, 24));
        painter.drawLine(QPointF(13, 31), QPointF(27, 31));
    } else if (kind == "link") {
        painter.setBrush(color);
        painter.drawEllipse(QPointF(10, 34), 3.5, 3.5);
        painter.drawEllipse(QPointF(38, 14), 3.5, 3.5);
        painter.drawLine(QPointF(13, 32), QPointF(35, 16));
    } else if (kind == "selection") {
        painter.setBrush(QColor(255, 255, 255, 210));
        QPolygonF pointer; pointer << QPointF(13, 8) << QPointF(36, 28)
                                  << QPointF(25, 29) << QPointF(20, 40);
        painter.drawPolygon(pointer);
    } else if (kind == "fit") {
        painter.drawLine(QPointF(8, 19), QPointF(8, 8)); painter.drawLine(QPointF(8, 8), QPointF(19, 8));
        painter.drawLine(QPointF(40, 19), QPointF(40, 8)); painter.drawLine(QPointF(40, 8), QPointF(29, 8));
        painter.drawLine(QPointF(8, 29), QPointF(8, 40)); painter.drawLine(QPointF(8, 40), QPointF(19, 40));
        painter.drawLine(QPointF(40, 29), QPointF(40, 40)); painter.drawLine(QPointF(40, 40), QPointF(29, 40));
    } else if (kind == "panel") {
        painter.drawRoundedRect(QRectF(7, 8, 34, 32), 3, 3);
        painter.drawLine(QPointF(7, 17), QPointF(41, 17));
        painter.drawLine(QPointF(28, 17), QPointF(28, 40));
    }
    return QIcon(pixmap);
}

static QToolButton *toolButton(const QIcon &icon, const QString &text, QWidget *parent,
                               const QString &role = QStringLiteral("command"))
{
    auto *button = new QToolButton(parent);
    button->setIcon(icon);
    button->setIconSize(QSize(25, 25));
    button->setText(text);
    button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    button->setProperty("toolRole", role);
    button->setMinimumSize(62, 58);
    button->setAutoRaise(false);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

static QFrame *toolbarGroup(const QString &title, QLayout *content, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName("toolbarGroup");
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(8, 3, 8, 5);
    layout->setSpacing(3);
    auto *label = new QLabel(title, frame);
    label->setObjectName("toolbarGroupTitle");
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(label);
    layout->addLayout(content, 1);
    return frame;
}

void MainWindow::setupUi()
{
    resize(1120, 720);
    setMinimumSize(900, 580);
    setWindowTitle("OpenLooping");
    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
    setStyleSheet(R"(
        QMainWindow { background: #e7ebf0; }
        QWidget { font-family: "DejaVu Sans"; font-size: 10px; color: #263445; }
        #topBar { background: #ffffff; border-bottom: 1px solid #d8e0e8; }
        #brandMark { background: #2563eb; color: white; border-radius: 9px; font-size: 13px; font-weight: 800; }
        #brandName { color: #162033; font-size: 14px; font-weight: 700; }
        #workspacePill { background: #eaf1ff; color: #1d4ed8; border: 1px solid #cbdcff; border-radius: 10px; padding: 3px 10px; font-weight: 700; }
        #toolbarGroup { border-right: 1px solid #e6ebf0; background: transparent; }
        #toolbarGroupTitle { color: #8491a3; font-size: 8px; font-weight: 700; }
        QToolButton[toolRole="command"] { min-width: 54px; border: 1px solid transparent; border-radius: 8px; padding: 3px 5px; background: transparent; color: #344256; }
        QToolButton[toolRole="command"]:hover { border-color: #d3e1f5; background: #f2f7ff; color: #174ea6; }
        QToolButton[toolRole="command"]:pressed, QToolButton[toolRole="command"]:checked { border-color: #adc8f5; background: #e5efff; color: #174ea6; }
        QToolButton[toolRole="primary"] { min-width: 60px; border: 1px solid #1d4ed8; border-radius: 8px; padding: 3px 7px; background: #2563eb; color: white; font-weight: 700; }
        QToolButton[toolRole="primary"]:hover { background: #1d4ed8; }
        QToolButton[toolRole="toggle"] { border: 1px solid #d8e0e8; border-radius: 7px; padding: 4px 9px; background: #f8fafc; color: #526176; }
        QToolButton[toolRole="toggle"]:hover { border-color: #b9cbe2; background: #f2f7ff; }
        QToolButton[toolRole="toggle"]:checked { border-color: #9cbcf0; background: #e7f0ff; color: #1d4ed8; font-weight: 700; }
        #fileButton { border: 1px solid #d7e0e9; border-radius: 8px; padding: 5px 11px; background: #f8fafc; font-weight: 700; }
        #fileButton:hover { border-color: #aac4e8; background: #f0f6ff; }
        #fileButton::menu-indicator { image: none; width: 0px; }
        #zoomLabel { color: #64748b; font-size: 9px; font-weight: 700; }
        QSlider::groove:horizontal { height: 4px; background: #dce4ed; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #3b82f6; border-radius: 2px; }
        QSlider::handle:horizontal { width: 14px; margin: -5px 0; border-radius: 7px; background: white; border: 2px solid #3b82f6; }
        QMenu { background: white; border: 1px solid #d6dee8; border-radius: 8px; padding: 5px; }
        QMenu::item { padding: 7px 28px 7px 10px; border-radius: 5px; }
        QMenu::item:selected { background: #eaf2ff; color: #174ea6; }
        QPushButton { border: 1px solid #cfd8e2; border-radius: 6px; padding: 5px 9px; background: #ffffff; }
        QPushButton:hover { border-color: #9dbbe0; background: #f0f6ff; }
        QMdiArea { background: #e3e8ee; }
        QMdiSubWindow { background: #f7f8fa; }
        QDockWidget::title { background: #f4f7fa; border: 1px solid #d5dde6; padding: 6px; text-align: left; }
        QStatusBar { background: #fbfcfd; border-top: 1px solid #d7dee6; color: #526176; }
    )");

    auto *root = new QWidget;
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *topBar = new QFrame(root);
    topBar->setObjectName("topBar");
    auto *barLayout = new QVBoxLayout(topBar);
    barLayout->setContentsMargins(12, 7, 12, 7);
    barLayout->setSpacing(5);

    auto *header = new QHBoxLayout;
    header->setSpacing(8);
    auto *brandMark = new QLabel("OL", topBar); brandMark->setObjectName("brandMark"); brandMark->setFixedSize(34, 34); brandMark->setAlignment(Qt::AlignCenter);
    auto *brandName = new QLabel("OpenLooping", topBar); brandName->setObjectName("brandName");
    auto *workspace = new QLabel(tr("MODÈLE CONCEPTUEL"), topBar); workspace->setObjectName("workspacePill");
    auto headerAction = [topBar](const QIcon &icon, const QString &tooltip) {
        auto *button = new QToolButton(topBar); button->setIcon(icon); button->setIconSize(QSize(18, 18));
        button->setToolTip(tooltip); button->setAccessibleName(tooltip); button->setProperty("toolRole", "toggle"); button->setFixedSize(31, 31);
        button->setCursor(Qt::PointingHandCursor); return button;
    };
    auto *newButton = headerAction(domainIcon("new", QColor("#475569")), tr("Nouveau (Ctrl+N)"));
    auto *openButton = headerAction(domainIcon("open", QColor("#475569")), tr("Ouvrir (Ctrl+O)"));
    auto *saveButton = headerAction(domainIcon("save", QColor("#475569")), tr("Enregistrer (Ctrl+S)"));
    auto *edition = new QToolButton(topBar); edition->setObjectName("fileButton"); edition->setText(tr("Édition  ▾")); edition->setPopupMode(QToolButton::InstantPopup); edition->setCursor(Qt::PointingHandCursor);
    auto *advancedTools = new QToolButton(topBar); advancedTools->setObjectName("fileButton"); advancedTools->setText(tr("Outils avancés  ▾")); advancedTools->setPopupMode(QToolButton::InstantPopup); advancedTools->setCursor(Qt::PointingHandCursor);
    auto *windows = new QToolButton(topBar); windows->setObjectName("fileButton"); windows->setText(tr("Fenêtres  ▾")); windows->setPopupMode(QToolButton::InstantPopup); windows->setCursor(Qt::PointingHandCursor);
    auto *fileButton = new QToolButton(topBar); fileButton->setObjectName("fileButton"); fileButton->setText(tr("Fichier  ▾")); fileButton->setPopupMode(QToolButton::InstantPopup); fileButton->setCursor(Qt::PointingHandCursor);
    header->addWidget(brandMark); header->addWidget(brandName); header->addSpacing(8); header->addWidget(workspace); header->addStretch();
    header->addWidget(newButton); header->addWidget(openButton); header->addWidget(saveButton); header->addSpacing(8);
    header->addWidget(edition); header->addWidget(advancedTools); header->addWidget(windows); header->addWidget(fileButton);
    barLayout->addLayout(header);

    auto *groups = new QHBoxLayout;
    groups->setContentsMargins(0, 0, 0, 0);
    groups->setSpacing(0);

    auto *modelTools = new QHBoxLayout; modelTools->setSpacing(2);
    auto *selection = toolButton(domainIcon("selection"), tr("Sélection"), topBar);
    auto *entityButton = toolButton(domainIcon("entity", QColor("#2563eb")), tr("Entité"), topBar);
    auto *associationButton = toolButton(domainIcon("association", QColor("#0891b2")), tr("Association"), topBar);
    auto *inheritanceButton = toolButton(domainIcon("inheritance", QColor("#7c3aed")), tr("Héritage"), topBar);
    auto *ruleButton = toolButton(domainIcon("rule", QColor("#b7791f")), tr("Règle"), topBar);
    auto *linkButton = toolButton(domainIcon("link", QColor("#2563eb")), tr("Lien"), topBar);
    m_modeButtons = new QButtonGroup(this);
    m_modeButtons->setExclusive(true);
    const QList<QToolButton *> modeButtons{selection, entityButton, associationButton, inheritanceButton, ruleButton, linkButton};
    for (int index = 0; index < modeButtons.size(); ++index) {
        modeButtons[index]->setCheckable(true);
        m_modeButtons->addButton(modeButtons[index], index);
    }
    selection->setChecked(true);
    modelTools->addWidget(selection); modelTools->addWidget(entityButton); modelTools->addWidget(associationButton);
    modelTools->addWidget(inheritanceButton); modelTools->addWidget(ruleButton); modelTools->addWidget(linkButton);
    groups->addWidget(toolbarGroup(tr("CONSTRUCTION"), modelTools, topBar));

    auto *display = new QGridLayout; display->setHorizontalSpacing(6); display->setVerticalSpacing(5);
    auto *rubriques = toolButton(QIcon(), tr("Rubriques"), topBar, QStringLiteral("toggle")); rubriques->setToolButtonStyle(Qt::ToolButtonTextOnly); rubriques->setCheckable(true); rubriques->setChecked(true); rubriques->setMinimumSize(78, 25);
    auto *types = toolButton(QIcon(), tr("Types"), topBar, QStringLiteral("toggle")); types->setToolButtonStyle(Qt::ToolButtonTextOnly); types->setCheckable(true); types->setMinimumSize(58, 25);
    auto *fit = toolButton(domainIcon("fit", QColor("#475569")), tr("Ajuster"), topBar);
    m_zoomLabel = new QLabel(tr("ZOOM  100%")); m_zoomLabel->setObjectName("zoomLabel");
    m_zoomSlider = new QSlider(Qt::Horizontal); m_zoomSlider->setRange(25, 200); m_zoomSlider->setValue(100); m_zoomSlider->setFixedWidth(112);
    display->addWidget(rubriques, 0, 0); display->addWidget(types, 1, 0);
    display->addWidget(fit, 0, 1, 2, 1); display->addWidget(m_zoomLabel, 0, 2); display->addWidget(m_zoomSlider, 1, 2);
    groups->addWidget(toolbarGroup(tr("AFFICHAGE"), display, topBar));

    auto *panelTools = new QHBoxLayout; panelTools->setSpacing(2);
    auto *mldButton = toolButton(domainIcon("panel", QColor("#0f766e")), "MLD", topBar);
    auto *sqlButton = toolButton(domainIcon("panel", QColor("#9333ea")), "LDD", topBar);
    panelTools->addWidget(mldButton); panelTools->addWidget(sqlButton);
    groups->addWidget(toolbarGroup(tr("PANNEAUX"), panelTools, topBar));
    groups->addStretch();
    barLayout->addLayout(groups);
    rootLayout->addWidget(topBar);

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
    fileButton->setMenu(fileMenu);

    auto *advancedMenu = new QMenu(this);
    advancedMenu->addSection(tr("Objets MCD"));
    auto addPlannedTool = [advancedMenu](const QString &name) {
        auto *action = advancedMenu->addAction(name + QObject::tr(" — à venir"));
        action->setEnabled(false);
    };
    addPlannedTool(tr("Contrainte inter-associations"));
    addPlannedTool(tr("CIF"));
    addPlannedTool(tr("Cassure de lien"));
    addPlannedTool(tr("Sous-modèle"));
    addPlannedTool(tr("Illustration"));
    advancedMenu->addSeparator();
    advancedMenu->addSection(tr("Représentations"));
    auto *eaView = advancedMenu->addAction(tr("MCD Entité / Association")); eaView->setCheckable(true); eaView->setChecked(true);
    addPlannedTool(tr("Diagramme UML"));
    addPlannedTool(tr("Modèle organisationnel de flux"));
    advancedTools->setMenu(advancedMenu);
    connect(newButton, &QToolButton::clicked, newAction, &QAction::trigger);
    connect(openButton, &QToolButton::clicked, openAction, &QAction::trigger);
    connect(saveButton, &QToolButton::clicked, saveAction, &QAction::trigger);

    connect(entityButton, &QToolButton::clicked, this, &MainWindow::addEntity);
    connect(associationButton, &QToolButton::clicked, this, &MainWindow::addAssociation);
    connect(inheritanceButton, &QToolButton::clicked, this, &MainWindow::addInheritance);
    connect(ruleButton, &QToolButton::clicked, this, &MainWindow::addRule);
    connect(linkButton, &QToolButton::clicked, this, &MainWindow::startLink);
    connect(mldButton, &QToolButton::clicked, this, [this] { m_mldDock->show(); });
    connect(sqlButton, &QToolButton::clicked, this, [this] { m_sqlDock->show(); });
    connect(selection, &QToolButton::clicked, this, &MainWindow::cancelCurrentTool);
    connect(rubriques, &QToolButton::toggled, this, [this](bool visible) { m_showFields = visible; rebuildScene(); });
    connect(types, &QToolButton::toggled, this, [this](bool visible) { m_showTypes = visible; rebuildScene(); });
    connect(m_zoomSlider, &QSlider::valueChanged, this, [this](int value) {
        m_view->resetTransform(); m_view->scale(value / 100.0, value / 100.0);
        m_zoomLabel->setText(tr("ZOOM  %1%").arg(value));
    });
    connect(m_view, &DiagramView::zoomSteps, this, [this](int steps) { m_zoomSlider->setValue(m_zoomSlider->value() + steps * 10); });
    connect(fit, &QToolButton::clicked, this, &MainWindow::fitModelToView);

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
    if (m_modeButtons && m_modeButtons->button(0)) m_modeButtons->button(0)->setChecked(true);
    statusBar()->clearMessage();
}

void MainWindow::newModel()
{
    if (!confirmDiscard()) return;
    m_model = {}; m_path.clear(); m_dirty = false; m_linkMode = false;
    m_linkStartKind.clear(); m_linkStartId.clear(); m_pendingPlacement.clear();
    m_view->setConnectionMode(false);
    if (m_modeButtons && m_modeButtons->button(0)) m_modeButtons->button(0)->setChecked(true);
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
    if (m_modeButtons && m_modeButtons->button(0)) m_modeButtons->button(0)->setChecked(true);
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
    if (m_modeButtons && m_modeButtons->button(1)) m_modeButtons->button(1)->setChecked(true);
    m_pendingPlacement = "entity"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer l’entité"));
}

void MainWindow::placeObject(const QPointF &position)
{
    if (m_pendingPlacement.isEmpty()) return;
    const QString kind = m_pendingPlacement;
    m_pendingPlacement.clear();
    if (m_modeButtons && m_modeButtons->button(0)) m_modeButtons->button(0)->setChecked(true);
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
    if (m_modeButtons && m_modeButtons->button(2)) m_modeButtons->button(2)->setChecked(true);
    m_pendingPlacement = "association"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer l’association"));
}

void MainWindow::addInheritance()
{
    if (m_modeButtons && m_modeButtons->button(3)) m_modeButtons->button(3)->setChecked(true);
    m_pendingPlacement = "inheritance"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer l’héritage"));
}

void MainWindow::addRule()
{
    if (m_modeButtons && m_modeButtons->button(4)) m_modeButtons->button(4)->setChecked(true);
    m_pendingPlacement = "rule"; m_linkMode = false; m_linkStartKind.clear(); m_linkStartId.clear();
    m_view->setConnectionMode(false);
    statusBar()->showMessage(tr("Cliquez sur le canevas pour placer la règle"));
}

void MainWindow::startLink()
{
    if (m_modeButtons && m_modeButtons->button(5)) m_modeButtons->button(5)->setChecked(true);
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
