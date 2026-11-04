#include "core/persistence.h"
#include "core/loopingimporter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace looping {

static QJsonObject fieldJson(const Field &f)
{
    return {{"id", f.id}, {"name", f.name}, {"logicalName", f.logicalName},
            {"type", static_cast<int>(f.type)}, {"length", f.length},
            {"primaryKey", f.primaryKey}, {"notNull", f.primaryKey || f.notNull}, {"unique", f.unique},
            {"precision", f.precision}, {"scale", f.scale}, {"collation", f.collation},
            {"complement", f.complement}, {"freeType", f.freeType}, {"comment", f.comment}};
}

static Field readField(const QJsonObject &o)
{
    Field field{o["id"].toString(), o["name"].toString(), o["logicalName"].toString(),
            static_cast<DataType>(o["type"].toInt()), o["length"].toInt(50),
            o["primaryKey"].toBool(), o["notNull"].toBool(), o["unique"].toBool()};
    field.precision = o["precision"].toInt(10);
    field.scale = o["scale"].toInt(2);
    field.collation = o["collation"].toString();
    field.complement = o["complement"].toString();
    field.freeType = o["freeType"].toString();
    field.comment = o["comment"].toString();
    if (field.primaryKey) field.notNull = true;
    return field;
}

bool Persistence::save(const Model &model, const QString &path, QString *error)
{
    QJsonObject root{{"format", "openlooping"}, {"version", 1}, {"name", model.name}};
    QJsonArray entities;
    for (const auto &e : model.entities) {
        QJsonArray fields;
        for (const auto &f : e.fields) fields.append(fieldJson(f));
        entities.append(QJsonObject{{"id", e.id}, {"name", e.name}, {"logicalName", e.logicalName},
                                    {"x", e.position.x()}, {"y", e.position.y()}, {"fields", fields}});
    }
    root["entities"] = entities;
    QJsonArray associations;
    for (const auto &a : model.associations) {
        QJsonArray fields;
        for (const auto &f : a.fields) fields.append(fieldJson(f));
        associations.append(QJsonObject{{"id", a.id}, {"name", a.name}, {"logicalName", a.logicalName},
                                        {"x", a.position.x()}, {"y", a.position.y()}, {"fields", fields}});
    }
    root["associations"] = associations;
    QJsonArray links;
    for (const auto &l : model.links)
        links.append(QJsonObject{{"id", l.id}, {"entityId", l.entityId}, {"associationId", l.associationId},
                                 {"cardinality", static_cast<int>(l.cardinality)}, {"customMin", l.customMin},
                                 {"customMax", l.customMax}, {"role", l.role},
                                 {"foreignKeyPrimaryPosition", l.foreignKeyPrimaryPosition}});
    root["links"] = links;
    QJsonArray inheritances;
    for (const auto &h : model.inheritances)
        inheritances.append(QJsonObject{{"id", h.id}, {"name", h.name}, {"x", h.position.x()}, {"y", h.position.y()},
                                        {"constraint", static_cast<int>(h.constraint)}, {"migrateAllFields", h.migrateAllFields},
                                        {"comment", h.comment}, {"sqlScript", h.sqlScript}});
    root["inheritances"] = inheritances;
    QJsonArray inheritanceLinks;
    for (const auto &l : model.inheritanceLinks)
        inheritanceLinks.append(QJsonObject{{"id", l.id}, {"entityId", l.entityId}, {"inheritanceId", l.inheritanceId},
            {"supertype", l.supertype}, {"expression", l.expression}, {"display", l.display},
            {"suffixForeignKeys", l.suffixForeignKeys}, {"renamedForeignKey", l.renamedForeignKey},
            {"foreignKeyPrimaryPosition", l.foreignKeyPrimaryPosition}, {"onDeleteCascade", l.onDeleteCascade},
            {"onUpdateCascade", l.onUpdateCascade}, {"otherForeignKeySql", l.otherForeignKeySql}, {"smoothBreak", l.smoothBreak}});
    root["inheritanceLinks"] = inheritanceLinks;
    QJsonArray rules;
    for (const auto &r : model.rules)
        rules.append(QJsonObject{{"id", r.id}, {"name", r.name}, {"x", r.position.x()}, {"y", r.position.y()},
            {"description", r.description}, {"showDescription", r.showDescription},
            {"addDescriptionAsSqlComment", r.addDescriptionAsSqlComment}, {"sqlScript", r.sqlScript},
            {"sqlPosition", static_cast<int>(r.sqlPosition)}, {"showSqlInModel", r.showSqlInModel}});
    root["rules"] = rules;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool Persistence::isNativeLoopingFile(const QString &path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) && LoopingImporter::resemblesNativeArchive(file.readAll());
}

bool Persistence::load(Model &model, const QString &path, QString *error, QStringList *warnings)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    const QByteArray contents = file.readAll();
    if (LoopingImporter::resemblesNativeArchive(contents))
        return LoopingImporter::load(model, contents, path, error, warnings);

    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(contents, &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = parse.errorString();
        return false;
    }
    const auto root = document.object();
    if (root["format"].toString() != "openlooping") {
        if (error) *error = QStringLiteral("Format de fichier non reconnu");
        return false;
    }
    Model loaded;
    loaded.name = root["name"].toString();
    for (const auto value : root["entities"].toArray()) {
        const auto o = value.toObject();
        Entity e{o["id"].toString(), o["name"].toString(), o["logicalName"].toString(),
                 {o["x"].toDouble(), o["y"].toDouble()}, {}};
        for (const auto f : o["fields"].toArray()) e.fields.append(readField(f.toObject()));
        loaded.entities.append(e);
    }
    for (const auto value : root["associations"].toArray()) {
        const auto o = value.toObject();
        Association a{o["id"].toString(), o["name"].toString(), o["logicalName"].toString(),
                      {o["x"].toDouble(), o["y"].toDouble()}, {}};
        for (const auto f : o["fields"].toArray()) a.fields.append(readField(f.toObject()));
        loaded.associations.append(a);
    }
    for (const auto value : root["links"].toArray()) {
        const auto o = value.toObject();
        loaded.links.append({o["id"].toString(), o["entityId"].toString(), o["associationId"].toString(),
                             static_cast<Cardinality>(o["cardinality"].toInt()), o["customMin"].toString(),
                             o["customMax"].toString(), o["role"].toString(),
                             o["foreignKeyPrimaryPosition"].toInt()});
    }
    for (const auto value : root["inheritances"].toArray()) {
        const auto o = value.toObject();
        loaded.inheritances.append({o["id"].toString(), o["name"].toString(), {o["x"].toDouble(), o["y"].toDouble()},
            static_cast<InheritanceConstraint>(o["constraint"].toInt()), o["migrateAllFields"].toBool(),
            o["comment"].toString(), o["sqlScript"].toString()});
    }
    for (const auto value : root["inheritanceLinks"].toArray()) {
        const auto o = value.toObject();
        loaded.inheritanceLinks.append({o["id"].toString(), o["entityId"].toString(), o["inheritanceId"].toString(),
            o["supertype"].toBool(), o["expression"].toString(), o["display"].toBool(true),
            o["suffixForeignKeys"].toBool(), o["renamedForeignKey"].toString(), o["foreignKeyPrimaryPosition"].toInt(),
            o["onDeleteCascade"].toBool(), o["onUpdateCascade"].toBool(), o["otherForeignKeySql"].toString(), o["smoothBreak"].toBool()});
    }
    for (const auto value : root["rules"].toArray()) {
        const auto o = value.toObject();
        loaded.rules.append({o["id"].toString(), o["name"].toString(), {o["x"].toDouble(), o["y"].toDouble()},
            o["description"].toString(), o["showDescription"].toBool(true), o["addDescriptionAsSqlComment"].toBool(),
            o["sqlScript"].toString(), static_cast<SqlRulePosition>(o["sqlPosition"].toInt(1)), o["showSqlInModel"].toBool()});
    }
    model = std::move(loaded);
    return true;
}

} // namespace looping
