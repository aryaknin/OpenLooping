#include "ui/objectdialog.h"

#include <QCheckBox>
#include <QTableWidget>
#include <QtTest>

class UiTests : public QObject {
    Q_OBJECT
private slots:
    void identifierChecksAndLocksNotNull();
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

QTEST_MAIN(UiTests)
#include "ui_tests.moc"
