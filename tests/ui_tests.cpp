#include "ui/objectdialog.h"
#include "ui/diagramview.h"
#include "ui/mainwindow.h"

#include "core/persistence.h"

#include <QCheckBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QtTest>

class UiTests : public QObject {
    Q_OBJECT
private slots:
    void identifierChecksAndLocksNotNull();
    void linkToolAcceptsBothOrdersAndStaysActive();
    void linkToolShowsAndClearsConnectionPreview();
    void linksHaveComfortableHitAreaAndDisplayRole();
};

void UiTests::identifierChecksAndLocksNotNull()
{
    ObjectDialog dialog(QStringLiteral("Entité"));
    looping::Field field{looping::newId(), QStringLiteral("idPersonne"),
                         QStringLiteral("idPersonne"), looping::DataType::Integer};
    dialog.setObject(QStringLiteral("Personne"), QStringLiteral("Personne"), {field});
    auto *table = dialog.findChild<QTableWidget *>();
    QVERIFY(table);
    auto *identifier = qobject_cast<QCheckBox *>(table->cellWidget(0, 0));
    auto *notNull = qobject_cast<QCheckBox *>(table->cellWidget(0, 4));
    QVERIFY(identifier); QVERIFY(notNull);
    QVERIFY(!notNull->isChecked()); QVERIFY(notNull->isEnabled());

    identifier->setChecked(true);

    QVERIFY(notNull->isChecked());
    QVERIFY(!notNull->isEnabled());
    const auto fields = dialog.fields();
    QVERIFY(fields[0].primaryKey);
    QVERIFY(fields[0].notNull);
}

void UiTests::linkToolAcceptsBothOrdersAndStaysActive()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    looping::Model model;
    model.name = QStringLiteral("Liens");
    model.entities = {
        {QStringLiteral("client"), QStringLiteral("Client"), QStringLiteral("Client"), {20, 20}, {}},
        {QStringLiteral("commande"), QStringLiteral("Commande"), QStringLiteral("Commande"), {20, 220}, {}}
    };
    model.associations = {
        {QStringLiteral("passe"), QStringLiteral("passe"), QStringLiteral("passe"), {300, 120}, {}}
    };
    const QString path = directory.filePath(QStringLiteral("links.lrea"));
    QString error;
    QVERIFY2(looping::Persistence::save(model, path, &error), qPrintable(error));

    MainWindow window;
    QVERIFY(window.loadModel(path, false));
    window.show();
    QCoreApplication::processEvents();
    auto *view = window.findChild<DiagramView *>();
    QVERIFY(view);

    auto linkCount = [view] {
        int count = 0;
        for (auto *item : view->scene()->items())
            if (item->data(0).toString() == QStringLiteral("link")) ++count;
        return count;
    };

    auto clickObject = [view](const QString &kind, const QString &id) {
        for (auto *item : view->scene()->items()) {
            if (item->data(0).toString() != kind || item->data(1).toString() != id) continue;
            QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                              view->mapFromScene(item->sceneBoundingRect().center()));
            QCoreApplication::processEvents();
            return true;
        }
        return false;
    };

    QVERIFY(QMetaObject::invokeMethod(&window, "startLink", Qt::DirectConnection));
    QVERIFY(clickObject(QStringLiteral("association"), QStringLiteral("passe")));
    QVERIFY(clickObject(QStringLiteral("entity"), QStringLiteral("client")));
    QCOMPARE(linkCount(), 1);

    // Le mode reste actif : un second lien se crée sans recliquer l'outil.
    QVERIFY(clickObject(QStringLiteral("entity"), QStringLiteral("commande")));
    QVERIFY(clickObject(QStringLiteral("association"), QStringLiteral("passe")));
    QCOMPARE(linkCount(), 2);

    // Un doublon est refusé et ne désactive pas l'outil.
    QVERIFY(clickObject(QStringLiteral("entity"), QStringLiteral("client")));
    QVERIFY(clickObject(QStringLiteral("association"), QStringLiteral("passe")));
    QCOMPARE(linkCount(), 2);

    QTest::keyClick(view, Qt::Key_Escape);
    QCOMPARE(view->viewport()->cursor().shape(), Qt::ArrowCursor);
}

void UiTests::linkToolShowsAndClearsConnectionPreview()
{
    QGraphicsScene scene;
    DiagramView view;
    view.setScene(&scene);
    auto *entity = scene.addRect(QRectF(20, 20, 120, 70));
    entity->setData(0, QStringLiteral("entity"));
    entity->setData(1, QStringLiteral("client"));

    view.setConnectionMode(true);
    view.setConnectionStart(QStringLiteral("entity"), QStringLiteral("client"));
    view.resize(480, 320);
    view.show();
    QCoreApplication::processEvents();

    QImage preview(view.viewport()->size(), QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::transparent);
    view.viewport()->render(&preview);
    bool hasBluePreview = false;
    for (int y = 0; y < preview.height() && !hasBluePreview; ++y)
        for (int x = 0; x < preview.width(); ++x) {
            const QColor color = preview.pixelColor(x, y);
            if (color.blue() > 170 && color.blue() > color.red() * 1.4) {
                hasBluePreview = true;
                break;
            }
        }
    QVERIFY(hasBluePreview);

    view.setConnectionMode(false);
    QCOMPARE(view.viewport()->cursor().shape(), Qt::ArrowCursor);
}

void UiTests::linksHaveComfortableHitAreaAndDisplayRole()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    looping::Model model;
    model.entities = {
        {QStringLiteral("client"), QStringLiteral("Client"), QStringLiteral("Client"), {20, 20}, {}}
    };
    model.associations = {
        {QStringLiteral("passe"), QStringLiteral("passe"), QStringLiteral("passe"), {320, 20}, {}}
    };
    looping::Link link{QStringLiteral("lien"), QStringLiteral("client"), QStringLiteral("passe")};
    link.role = QStringLiteral("commanditaire");
    model.links = {link};
    const QString path = directory.filePath(QStringLiteral("role.lrea"));
    QString error;
    QVERIFY2(looping::Persistence::save(model, path, &error), qPrintable(error));

    MainWindow window;
    QVERIFY(window.loadModel(path, false));
    window.show();
    QCoreApplication::processEvents();
    auto *view = window.findChild<DiagramView *>();
    QVERIFY(view);

    QGraphicsItem *linkItem = nullptr;
    bool roleVisible = false;
    for (auto *item : view->scene()->items()) {
        if (item->data(0).toString() == QStringLiteral("link")) linkItem = item;
        if (auto *text = qgraphicsitem_cast<QGraphicsTextItem *>(item))
            roleVisible = roleVisible || text->toPlainText().contains(QStringLiteral("commanditaire"));
    }
    QVERIFY(linkItem);
    QVERIFY(roleVisible);

    const QRectF bounds = linkItem->shape().boundingRect();
    QVERIFY2(bounds.height() >= 13.0 || bounds.width() >= 13.0,
             "La zone cliquable du lien doit être sensiblement plus large que son trait visible");

    const QPointF center = linkItem->sceneBoundingRect().center();
    const QPointF offset = bounds.width() > bounds.height() ? QPointF(0, 5) : QPointF(5, 0);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                      view->mapFromScene(center + offset));
    QCoreApplication::processEvents();
    QVERIFY(linkItem->isSelected());
}

QTEST_MAIN(UiTests)
#include "ui_tests.moc"
