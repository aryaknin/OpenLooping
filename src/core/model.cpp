#include "core/model.h"

#include <QUuid>

namespace looping {

template <typename T>
static T *findById(QVector<T> &items, const QString &id)
{
    for (auto &item : items)
        if (item.id == id)
            return &item;
    return nullptr;
}

template <typename T>
static const T *findById(const QVector<T> &items, const QString &id)
{
    for (const auto &item : items)
        if (item.id == id)
            return &item;
    return nullptr;
}

const Entity *Model::entity(const QString &id) const { return findById(entities, id); }
Entity *Model::entity(const QString &id) { return findById(entities, id); }
const Association *Model::association(const QString &id) const { return findById(associations, id); }
Association *Model::association(const QString &id) { return findById(associations, id); }
const Link *Model::link(const QString &id) const { return findById(links, id); }
Link *Model::link(const QString &id) { return findById(links, id); }
const Inheritance *Model::inheritance(const QString &id) const { return findById(inheritances, id); }
Inheritance *Model::inheritance(const QString &id) { return findById(inheritances, id); }
const InheritanceLink *Model::inheritanceLink(const QString &id) const { return findById(inheritanceLinks, id); }
InheritanceLink *Model::inheritanceLink(const QString &id) { return findById(inheritanceLinks, id); }
const Rule *Model::rule(const QString &id) const { return findById(rules, id); }
Rule *Model::rule(const QString &id) { return findById(rules, id); }

QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

QString normalizedLogicalName(const QString &name)
{
    QString result = name.simplified();
    result.replace(' ', '_');
    return result;
}

QString dataTypeName(DataType type, int length)
{
    Field field;
    field.type = type;
    field.length = length;
    return dataTypeName(field);
}

QString dataTypeName(const Field &field)
{
    const int length = qMax(1, field.length);
    switch (field.type) {
    case DataType::VarChar: return QStringLiteral("VARCHAR(%1)").arg(length);
    case DataType::Integer: return QStringLiteral("INT");
    case DataType::Char: return QStringLiteral("CHAR(%1)").arg(length);
    case DataType::Text: return QStringLiteral("TEXT");
    case DataType::NVarChar: return QStringLiteral("NVARCHAR(%1)").arg(length);
    case DataType::NChar: return QStringLiteral("NCHAR(%1)").arg(length);
    case DataType::NText: return QStringLiteral("NTEXT");
    case DataType::VarBinary: return QStringLiteral("VARBINARY(%1)").arg(length);
    case DataType::Binary: return QStringLiteral("BINARY(%1)").arg(length);
    case DataType::Blob: return QStringLiteral("BLOB");
    case DataType::Decimal: return QStringLiteral("DECIMAL(%1,%2)").arg(qMax(1, field.precision)).arg(qMax(0, field.scale));
    case DataType::Real: return QStringLiteral("REAL");
    case DataType::Money: return QStringLiteral("DECIMAL(%1,%2)").arg(qMax(1, field.precision)).arg(qMax(0, field.scale));
    case DataType::Counter: return QStringLiteral("INT");
    case DataType::Date: return QStringLiteral("DATE");
    case DataType::Time: return QStringLiteral("TIME");
    case DataType::DateTime: return QStringLiteral("TIMESTAMP");
    case DataType::Boolean: return QStringLiteral("BOOLEAN");
    case DataType::Xml: return QStringLiteral("XML");
    case DataType::Geometry: return QStringLiteral("GEOMETRY");
    case DataType::Geography: return QStringLiteral("GEOGRAPHY");
    case DataType::Free: return field.freeType.trimmed().isEmpty() ? QStringLiteral("TEXT") : field.freeType.trimmed();
    }
    return QStringLiteral("TEXT");
}

QString cardinalityName(const Link &link)
{
    switch (link.cardinality) {
    case Cardinality::ZeroOne: return QStringLiteral("0,1");
    case Cardinality::OneOne: return QStringLiteral("1,1");
    case Cardinality::ZeroMany: return QStringLiteral("0,n");
    case Cardinality::OneMany: return QStringLiteral("1,n");
    case Cardinality::Custom: return link.customMin + ',' + link.customMax;
    }
    return {};
}

bool maximumIsMany(const Link &link)
{
    return link.cardinality == Cardinality::ZeroMany
        || link.cardinality == Cardinality::OneMany
        || (link.cardinality == Cardinality::Custom && link.customMax.compare("n", Qt::CaseInsensitive) == 0);
}

bool minimumIsOne(const Link &link)
{
    return link.cardinality == Cardinality::OneOne
        || link.cardinality == Cardinality::OneMany
        || (link.cardinality == Cardinality::Custom && link.customMin == "1");
}

QString inheritanceConstraintName(InheritanceConstraint constraint)
{
    switch (constraint) {
    case InheritanceConstraint::None: return {};
    case InheritanceConstraint::Exclusive: return QStringLiteral("X");
    case InheritanceConstraint::Total: return QStringLiteral("T");
    case InheritanceConstraint::Partition: return QStringLiteral("XT");
    }
    return {};
}

} // namespace looping
