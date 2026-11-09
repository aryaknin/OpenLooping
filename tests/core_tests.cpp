#include "core/persistence.h"
#include "core/relationalgenerator.h"

#include <QTemporaryDir>
#include <QtTest>

using namespace looping;

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void manyToManyCreatesJoinTable();
    void oneOneMigratesNotNullForeignKey();
    void zeroOneMigratesNullableForeignKey();
    void persistenceRoundTrip();
    void primaryKeyAlwaysImpliesNotNull();
    void completeTypesAreGeneratedAndSaved();
    void inheritanceRoundTripPreservesObservedProperties();
    void rulesPreservePropertiesAndSqlOrder();
    void importsNativeEmptyArchiveWithoutChangingSource();
    void importsNativeLooAndLo1SimpleEntity();
    void recoversComplexNativeArchiveWithoutChangingSource();
    void importsObservedNativeCardinalities();
};

static Model sample(Cardinality right)
{
    Model m;
    m.entities = {
        {"client", "Client", "Client", {0, 0}, {{"nom", "nom", "nom", DataType::VarChar, 50, true, false, false}}},
        {"commande", "Commande", "Commande", {400, 0}, {{"id", "idCommande", "idCommande", DataType::Integer, 32, true, false, false}}}
    };
    m.associations = {{"passe", "passe", "passe", {200, 0}, {}}};
    m.links = {{"l1", "client", "passe", Cardinality::ZeroMany}, {"l2", "commande", "passe", right}};
    return m;
}

void CoreTests::manyToManyCreatesJoinTable()
{
    const auto output = RelationalGenerator::generate(sample(Cardinality::ZeroMany));
    QVERIFY(output.sql.contains("CREATE TABLE passe"));
    QVERIFY(output.sql.contains("PRIMARY KEY(nom, idCommande)"));
    QVERIFY(output.sql.contains("FOREIGN KEY(nom) REFERENCES Client(nom)"));
}

void CoreTests::oneOneMigratesNotNullForeignKey()
{
    const auto output = RelationalGenerator::generate(sample(Cardinality::OneOne));
    QVERIFY(!output.sql.contains("CREATE TABLE passe"));
    QVERIFY(output.sql.contains("nom VARCHAR(50) NOT NULL"));
    QVERIFY(output.sql.contains("FOREIGN KEY(nom) REFERENCES Client(nom)"));
}

void CoreTests::zeroOneMigratesNullableForeignKey()
{
    const auto output = RelationalGenerator::generate(sample(Cardinality::ZeroOne));
    QVERIFY(output.sql.contains("nom VARCHAR(50),"));
    QVERIFY(!output.sql.contains("nom VARCHAR(50) NOT NULL"));
}

void CoreTests::persistenceRoundTrip()
{
    QTemporaryDir dir; QVERIFY(dir.isValid());
    const QString path = dir.filePath("model.lrea"); QString error;
    QVERIFY2(Persistence::save(sample(Cardinality::OneOne), path, &error), qPrintable(error));
    Model loaded; QVERIFY2(Persistence::load(loaded, path, &error), qPrintable(error));
    QCOMPARE(loaded.entities.size(), 2); QCOMPARE(loaded.links.size(), 2);
    QCOMPARE(loaded.links[1].cardinality, Cardinality::OneOne);
}

void CoreTests::primaryKeyAlwaysImpliesNotNull()
{
    Model model;
    model.entities = {{"e", "Personne", "Personne", {0, 0},
                       {{"id", "idPersonne", "idPersonne", DataType::Integer, 32, true, false, false}}}};
    QTemporaryDir dir; QVERIFY(dir.isValid()); QString error;
    const QString path = dir.filePath("primary-key.lrea");
    QVERIFY2(Persistence::save(model, path, &error), qPrintable(error));
    Model loaded; QVERIFY2(Persistence::load(loaded, path, &error), qPrintable(error));
    QVERIFY(loaded.entities[0].fields[0].primaryKey);
    QVERIFY(loaded.entities[0].fields[0].notNull);
}

void CoreTests::completeTypesAreGeneratedAndSaved()
{
    Model model;
    Field amount{"amount", "montant", "montant", DataType::Decimal, 50, false, true, false};
    amount.precision = 12; amount.scale = 3; amount.comment = "Montant facturé";
    Field created{"created", "créé le", "cree_le", DataType::DateTime};
    Field active{"active", "actif", "actif", DataType::Boolean}; active.unique = true;
    model.entities = {{"invoice", "Facture", "Facture", {0, 0}, {amount, created, active}}};
    const auto sql = RelationalGenerator::generate(model).sql;
    QVERIFY(sql.contains("montant DECIMAL(12,3) NOT NULL"));
    QVERIFY(sql.contains("cree_le TIMESTAMP"));
    QVERIFY(sql.contains("actif BOOLEAN"));
    QVERIFY(sql.contains("actif BOOLEAN UNIQUE"));

    QTemporaryDir dir; QVERIFY(dir.isValid()); QString error;
    const QString path = dir.filePath("types.lrea");
    QVERIFY2(Persistence::save(model, path, &error), qPrintable(error));
    Model loaded; QVERIFY2(Persistence::load(loaded, path, &error), qPrintable(error));
    QCOMPARE(loaded.entities[0].fields[0].type, DataType::Decimal);
    QCOMPARE(loaded.entities[0].fields[0].precision, 12);
    QCOMPARE(loaded.entities[0].fields[0].scale, 3);
    QCOMPARE(loaded.entities[0].fields[0].comment, QString("Montant facturé"));
}

void CoreTests::inheritanceRoundTripPreservesObservedProperties()
{
    Model model;
    model.entities = {{"person", "Personne", "Personne", {0, 0}, {}}, {"student", "Étudiant", "Etudiant", {200, 0}, {}}};
    model.inheritances = {{"heritage", "Statut personne", {100, 100}, InheritanceConstraint::Partition, true,
                           "Spécialisation totale et exclusive", "-- SQL héritage"}};
    model.inheritanceLinks = {
        {"parent", "person", "heritage", true, "", true, false, "", 0, false, false, "", false},
        {"child", "student", "heritage", false, "type = 'E'", true, true, "personne_id", 1, true, false, "", true}
    };
    QTemporaryDir dir; QVERIFY(dir.isValid()); QString error;
    const QString path = dir.filePath("inheritance.lrea");
    QVERIFY2(Persistence::save(model, path, &error), qPrintable(error));
    Model loaded; QVERIFY2(Persistence::load(loaded, path, &error), qPrintable(error));
    QCOMPARE(loaded.inheritances.size(), 1); QCOMPARE(loaded.inheritanceLinks.size(), 2);
    QCOMPARE(loaded.inheritances[0].constraint, InheritanceConstraint::Partition);
    QVERIFY(loaded.inheritances[0].migrateAllFields);
    QVERIFY(loaded.inheritanceLinks[0].supertype);
    QCOMPARE(loaded.inheritanceLinks[1].expression, QString("type = 'E'"));
    QVERIFY(loaded.inheritanceLinks[1].onDeleteCascade);
    QVERIFY(loaded.inheritanceLinks[1].smoothBreak);
    const auto generated = RelationalGenerator::generate(loaded);
    QVERIFY(generated.sql.contains("-- SQL héritage"));
    QVERIFY(!generated.warnings.filter("traduction de l’héritage").isEmpty());
}

void CoreTests::rulesPreservePropertiesAndSqlOrder()
{
    Model model;
    model.entities = {{"e", "Élève", "Eleve", {0, 0}, {{"id", "id", "id", DataType::Integer, 32, true, true, false}}}};
    model.rules = {
        {"before", "Préambule", {20, 30}, "Initialisation", true, true, "SET test = 1;", SqlRulePosition::BeforeTables, true},
        {"after", "Contrôle", {200, 30}, "", false, false, "CHECK FINAL;", SqlRulePosition::AfterTables, false}
    };
    const auto generated = RelationalGenerator::generate(model);
    QVERIFY(generated.sql.indexOf("-- Initialisation") < generated.sql.indexOf("CREATE TABLE"));
    QVERIFY(generated.sql.indexOf("CHECK FINAL") > generated.sql.indexOf("CREATE TABLE"));

    QTemporaryDir dir; QVERIFY(dir.isValid()); QString error; const QString path = dir.filePath("rules.lrea");
    QVERIFY2(Persistence::save(model, path, &error), qPrintable(error));
    Model loaded; QVERIFY2(Persistence::load(loaded, path, &error), qPrintable(error));
    QCOMPARE(loaded.rules.size(), 2); QCOMPARE(loaded.rules[0].sqlPosition, SqlRulePosition::BeforeTables);
    QVERIFY(loaded.rules[0].addDescriptionAsSqlComment); QVERIFY(loaded.rules[0].showSqlInModel);
    QVERIFY(!loaded.rules[1].showDescription);
}

static QString nativeSample(const char *name)
{
    return QStringLiteral(LOOPING_TEST_DATA_DIR) + QLatin1Char('/') + QString::fromLatin1(name);
}

void CoreTests::importsNativeEmptyArchiveWithoutChangingSource()
{
    const QString path = nativeSample("sample-empty.loo");
    QFile source(path); QVERIFY(source.open(QIODevice::ReadOnly)); const QByteArray before = source.readAll(); source.close();
    QVERIFY(Persistence::isNativeLoopingFile(path));
    Model loaded; QString error; QStringList warnings;
    QVERIFY2(Persistence::load(loaded, path, &error, &warnings), qPrintable(error));
    QVERIFY(loaded.entities.isEmpty()); QVERIFY(!warnings.isEmpty());
    QVERIFY(source.open(QIODevice::ReadOnly)); QCOMPARE(source.readAll(), before);
}

void CoreTests::importsNativeLooAndLo1SimpleEntity()
{
    for (const char *name : {"sample-one-entity.loo", "sample-one-entity.lo1"}) {
        Model loaded; QString error; QStringList warnings;
        QVERIFY2(Persistence::load(loaded, nativeSample(name), &error, &warnings), qPrintable(error));
        QCOMPARE(loaded.entities.size(), 1);
        QCOMPARE(loaded.entities[0].name, QString::fromUtf8("Entité 1"));
        QCOMPARE(loaded.entities[0].position, QPointF(230, 90));
    }
}

void CoreTests::recoversComplexNativeArchiveWithoutChangingSource()
{
    const QString path = nativeSample("sample-client-commande-asso-default-links.loo");
    QFile source(path); QVERIFY(source.open(QIODevice::ReadOnly)); const QByteArray before = source.readAll(); source.close();
    Model model; QString error; QStringList warnings;
    QVERIFY2(Persistence::load(model, path, &error, &warnings), qPrintable(error));
    QCOMPARE(model.entities.size(), 2);
    QCOMPARE(model.entities[0].name, QString("Client"));
    QCOMPARE(model.entities[0].fields.size(), 1);
    QCOMPARE(model.entities[0].fields[0].name, QString("nom"));
    QVERIFY(model.entities[0].fields[0].primaryKey);
    QVERIFY(model.entities[0].fields[0].notNull);
    QCOMPARE(model.entities[1].name, QString("Commande"));
    QCOMPARE(model.entities[1].fields[0].name, QString("idCommande"));
    QCOMPARE(model.associations.size(), 1);
    QCOMPARE(model.associations[0].name, QString("passe"));
    QCOMPARE(model.links.size(), 2);
    QCOMPARE(model.links[0].cardinality, Cardinality::ZeroMany);
    QVERIFY(warnings.join(' ').contains("IMPORT NATIF EN COURS"));
    QVERIFY(source.open(QIODevice::ReadOnly)); QCOMPARE(source.readAll(), before);
}

void CoreTests::importsObservedNativeCardinalities()
{
    const struct { const char *file; Cardinality second; } cases[] = {
        {"sample-client-commande-asso-0n-01.loo", Cardinality::ZeroOne},
        {"sample-client-commande-asso-0n-11.loo", Cardinality::OneOne},
    };
    for (const auto &test : cases) {
        Model model; QString error; QStringList warnings;
        QVERIFY2(Persistence::load(model, nativeSample(test.file), &error, &warnings), qPrintable(error));
        QCOMPARE(model.associations.size(), 1);
        QCOMPARE(model.associations[0].position, QPointF(470, 120));
        QCOMPARE(model.links.size(), 2);
        QCOMPARE(model.links[0].cardinality, Cardinality::ZeroMany);
        QCOMPARE(model.links[1].cardinality, test.second);
    }
}

QTEST_MAIN(CoreTests)
#include "core_tests.moc"
