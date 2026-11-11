#pragma once

#include <QPointF>
#include <QString>
#include <QVector>

namespace looping {

enum class DataType {
    VarChar = 0, Integer = 1,
    Char, Text, NVarChar, NChar, NText, VarBinary, Binary, Blob,
    Decimal, Real, Money, Counter, Date, Time, DateTime,
    Boolean, Xml, Geometry, Geography, Free
};
enum class Cardinality { ZeroOne, OneOne, ZeroMany, OneMany, Custom };
enum class InheritanceConstraint { None, Exclusive, Total, Partition };
enum class SqlRulePosition { BeforeTables, AfterTables };

struct Field {
    QString id;
    QString name;
    QString logicalName;
    DataType type = DataType::VarChar;
    int length = 50;
    bool primaryKey = false;
    bool notNull = false;
    bool unique = false;
    int precision = 10;
    int scale = 2;
    QString collation;
    QString complement;
    QString freeType;
    QString comment;
};

struct Entity {
    QString id;
    QString name;
    QString logicalName;
    QPointF position;
    QVector<Field> fields;
};

struct Association {
    QString id;
    QString name;
    QString logicalName;
    QPointF position;
    QVector<Field> fields;
};

struct Link {
    QString id;
    QString entityId;
    QString associationId;
    Cardinality cardinality = Cardinality::ZeroMany;
    QString customMin;
    QString customMax;
    QString role;
    int foreignKeyPrimaryPosition = 0;
    QVector<QPointF> breakpoints;
};

struct Inheritance {
    QString id;
    QString name;
    QPointF position;
    InheritanceConstraint constraint = InheritanceConstraint::None;
    bool migrateAllFields = false;
    QString comment;
    QString sqlScript;
};

struct InheritanceLink {
    QString id;
    QString entityId;
    QString inheritanceId;
    bool supertype = false;
    QString expression;
    bool display = true;
    bool suffixForeignKeys = false;
    QString renamedForeignKey;
    int foreignKeyPrimaryPosition = 0;
    bool onDeleteCascade = false;
    bool onUpdateCascade = false;
    QString otherForeignKeySql;
    bool smoothBreak = false;
};

struct Rule {
    QString id;
    QString name;
    QPointF position;
    QString description;
    bool showDescription = true;
    bool addDescriptionAsSqlComment = false;
    QString sqlScript;
    SqlRulePosition sqlPosition = SqlRulePosition::AfterTables;
    bool showSqlInModel = false;
};

struct Model {
    QString name = QStringLiteral("Nouveau modèle");
    QVector<Entity> entities;
    QVector<Association> associations;
    QVector<Link> links;
    QVector<Inheritance> inheritances;
    QVector<InheritanceLink> inheritanceLinks;
    QVector<Rule> rules;

    const Entity *entity(const QString &id) const;
    Entity *entity(const QString &id);
    const Association *association(const QString &id) const;
    Association *association(const QString &id);
    const Link *link(const QString &id) const;
    Link *link(const QString &id);
    const Inheritance *inheritance(const QString &id) const;
    Inheritance *inheritance(const QString &id);
    const InheritanceLink *inheritanceLink(const QString &id) const;
    InheritanceLink *inheritanceLink(const QString &id);
    const Rule *rule(const QString &id) const;
    Rule *rule(const QString &id);
};

QString newId();
QString normalizedLogicalName(const QString &name);
QString dataTypeName(DataType type, int length = 50);
QString dataTypeName(const Field &field);
QString cardinalityName(const Link &link);
bool maximumIsMany(const Link &link);
bool minimumIsOne(const Link &link);
QString inheritanceConstraintName(InheritanceConstraint constraint);

} // namespace looping
