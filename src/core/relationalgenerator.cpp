#include "core/relationalgenerator.h"

#include <QHash>

namespace looping {

struct Column {
    QString name;
    QString type;
    bool primary = false;
    bool notNull = false;
    bool unique = false;
    QString referenceTable;
    QString referenceColumn;
};

struct Table { QString name; QVector<Column> columns; };

static QVector<Field> primaryFields(const Entity &entity)
{
    QVector<Field> result;
    for (const auto &field : entity.fields)
        if (field.primaryKey) result.append(field);
    return result;
}

static void appendForeignKeys(Table &target, const Entity &source, bool primary, bool notNull)
{
    for (const auto &field : primaryFields(source)) {
        target.columns.append({field.logicalName, dataTypeName(field), primary, notNull, false,
                               source.logicalName, field.logicalName});
    }
}

GeneratedOutput RelationalGenerator::generate(const Model &model)
{
    QVector<Table> tables;
    QHash<QString, int> entityTable;
    GeneratedOutput output;

    for (const auto &entity : model.entities) {
        Table table{entity.logicalName, {}};
        for (const auto &field : entity.fields)
            table.columns.append({field.logicalName, dataTypeName(field),
                                  field.primaryKey, field.notNull, field.unique, {}, {}});
        if (primaryFields(entity).isEmpty())
            output.warnings << QStringLiteral("%1 : identifiant absent").arg(entity.name);
        entityTable.insert(entity.id, tables.size());
        tables.append(table);
    }

    for (const auto &association : model.associations) {
        QVector<const Link *> links;
        for (const auto &link : model.links)
            if (link.associationId == association.id) links.append(&link);
        if (links.size() < 2) {
            output.warnings << QStringLiteral("%1 : association non reliée").arg(association.name);
            continue;
        }

        QVector<const Link *> oneSides;
        for (const auto *link : links)
            if (!maximumIsMany(*link)) oneSides.append(link);

        if (oneSides.size() == 1) {
            const auto *receiverLink = oneSides.front();
            const auto *receiver = model.entity(receiverLink->entityId);
            if (!receiver) continue;
            Table &table = tables[entityTable.value(receiver->id)];
            for (const auto *otherLink : links) {
                if (otherLink == receiverLink) continue;
                if (const auto *source = model.entity(otherLink->entityId))
                    appendForeignKeys(table, *source, false, minimumIsOne(*receiverLink));
            }
            for (const auto &field : association.fields)
                table.columns.append({field.logicalName, dataTypeName(field), false,
                                      field.notNull, field.unique, {}, {}});
        } else if (oneSides.isEmpty()) {
            Table table{association.logicalName, {}};
            for (const auto *link : links)
                if (const auto *entity = model.entity(link->entityId))
                    appendForeignKeys(table, *entity, true, false);
            for (const auto &field : association.fields)
                table.columns.append({field.logicalName, dataTypeName(field),
                                      field.primaryKey, field.notNull, field.unique, {}, {}});
            tables.append(table);
        } else {
            output.warnings << QStringLiteral("%1 : traduction 1:1 pas encore prise en charge").arg(association.name);
        }
    }
    for (const auto &inheritance : model.inheritances)
        output.warnings << QStringLiteral("%1 : traduction de l’héritage à auditer").arg(inheritance.name);

    QStringList mldLines;
    QStringList sqlBlocks;
    QStringList beforeRuleBlocks, afterRuleBlocks;
    for (const auto &rule : model.rules) {
        QString script = rule.sqlScript.trimmed();
        if (script.isEmpty()) continue;
        if (rule.addDescriptionAsSqlComment && !rule.description.trimmed().isEmpty()) {
            QStringList comments;
            for (const auto &line : rule.description.split('\n')) comments << QStringLiteral("-- %1").arg(line);
            script.prepend(comments.join('\n') + '\n');
        }
        (rule.sqlPosition == SqlRulePosition::BeforeTables ? beforeRuleBlocks : afterRuleBlocks) << script;
    }
    for (const auto &table : tables) {
        QStringList mldColumns;
        QStringList sqlLines;
        QStringList primary;
        for (const auto &column : table.columns) {
            QString marker;
            if (!column.referenceTable.isEmpty()) marker += '#';
            mldColumns << marker + column.name + ' ' + column.type;
            sqlLines << QStringLiteral("    %1 %2%3%4").arg(column.name, column.type,
                column.notNull ? QStringLiteral(" NOT NULL") : QString(),
                column.unique && !column.primary ? QStringLiteral(" UNIQUE") : QString());
            if (column.primary) primary << column.name;
        }
        mldLines << QStringLiteral("%1 = (%2);").arg(table.name, mldColumns.join(", "));
        if (!primary.isEmpty()) sqlLines << QStringLiteral("    PRIMARY KEY(%1)").arg(primary.join(", "));
        for (const auto &column : table.columns)
            if (!column.referenceTable.isEmpty())
                sqlLines << QStringLiteral("    FOREIGN KEY(%1) REFERENCES %2(%3)")
                                .arg(column.name, column.referenceTable, column.referenceColumn);
        for (qsizetype i = 0; i + 1 < sqlLines.size(); ++i) sqlLines[i] += ',';
        sqlBlocks << QStringLiteral("CREATE TABLE %1(\n%2\n);").arg(table.name, sqlLines.join('\n'));
    }
    for (const auto &inheritance : model.inheritances)
        if (!inheritance.sqlScript.trimmed().isEmpty()) sqlBlocks << inheritance.sqlScript.trimmed();
    output.mld = mldLines.join('\n');
    QStringList orderedSql = beforeRuleBlocks; orderedSql << sqlBlocks; orderedSql << afterRuleBlocks;
    output.sql = orderedSql.join("\n\n");
    return output;
}

} // namespace looping
