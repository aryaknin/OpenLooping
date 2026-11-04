#include "core/loopingimporter.h"

#include <QFileInfo>
#include <QRectF>
#include <QSet>

#include <algorithm>

namespace looping {
namespace {

constexpr qsizetype StreamOffset = 0x0a20;

quint16 little16(const QByteArray &data, qsizetype offset)
{
    return quint16(quint8(data[offset])) | (quint16(quint8(data[offset + 1])) << 8);
}

qint32 little32(const QByteArray &data, qsizetype offset)
{
    return qint32(quint32(quint8(data[offset])) |
                  (quint32(quint8(data[offset + 1])) << 8) |
                  (quint32(quint8(data[offset + 2])) << 16) |
                  (quint32(quint8(data[offset + 3])) << 24));
}

QStringList archiveStrings(const QByteArray &data)
{
    QStringList result;
    for (qsizetype i = StreamOffset; i + 4 <= data.size();) {
        if (quint8(data[i]) != 0xff || quint8(data[i + 1]) != 0xfe ||
            quint8(data[i + 2]) != 0xff) {
            ++i;
            continue;
        }
        const qsizetype length = quint8(data[i + 3]);
        const qsizetype bytes = length * 2;
        if (length == 0 || i + 4 + bytes > data.size()) {
            i += 4;
            continue;
        }
        QString value;
        value.reserve(length);
        bool valid = true;
        for (qsizetype j = 0; j < length; ++j) {
            const quint16 code = little16(data, i + 4 + j * 2);
            if (code == 0 || (code < 0x20 && code != '\t')) {
                valid = false;
                break;
            }
            value.append(QChar(code));
        }
        if (valid && !value.trimmed().isEmpty()) result.append(value);
        i += 4 + bytes;
    }
    return result;
}

struct StringRecord {
    qsizetype offset;
    qsizetype end;
    QString value;
};

QVector<StringRecord> archiveStringRecords(const QByteArray &data, qsizetype begin, qsizetype end)
{
    QVector<StringRecord> result;
    for (qsizetype i = begin; i + 4 <= end;) {
        if (quint8(data[i]) != 0xff || quint8(data[i + 1]) != 0xfe ||
            quint8(data[i + 2]) != 0xff) {
            ++i;
            continue;
        }
        const qsizetype length = quint8(data[i + 3]);
        const qsizetype recordEnd = i + 4 + length * 2;
        if (length == 0 || recordEnd > end) {
            i += 4;
            continue;
        }
        QString value;
        bool valid = true;
        for (qsizetype j = 0; j < length; ++j) {
            const quint16 code = little16(data, i + 4 + j * 2);
            if (code < 0x20 && code != '\t') { valid = false; break; }
            value.append(QChar(code));
        }
        if (valid && !value.trimmed().isEmpty()) result.append({i, recordEnd, value});
        i = recordEnd;
    }
    return result;
}

bool plausibleRectangle(const QByteArray &data, qsizetype textObjectOffset, QRectF *rectangle)
{
    if (textObjectOffset < 16) return false;
    const qint32 left = little32(data, textObjectOffset - 16);
    const qint32 top = little32(data, textObjectOffset - 12);
    const qint32 right = little32(data, textObjectOffset - 8);
    const qint32 bottom = little32(data, textObjectOffset - 4);
    if (right <= left || bottom <= top || left < -100000 || top < -100000 ||
        right > 100000 || bottom > 100000) return false;
    if (rectangle) *rectangle = QRectF(QPointF(left, top), QPointF(right, bottom));
    return true;
}

bool plausibleSizedRectangle(const QByteArray &data, qsizetype offset, QRectF *rectangle)
{
    if (offset < 0 || offset + 16 > data.size()) return false;
    const qint32 x = little32(data, offset);
    const qint32 y = little32(data, offset + 4);
    const qint32 width = little32(data, offset + 8);
    const qint32 height = little32(data, offset + 12);
    if (x < -100000 || y < -100000 || x > 100000 || y > 100000 ||
        width <= 0 || height <= 0 || width > 5000 || height > 5000) return false;
    if (rectangle) *rectangle = QRectF(x, y, width, height);
    return true;
}

bool isDisplayNoise(const QString &value)
{
    const QString text = value.trimmed();
    if (text.isEmpty() || text == QStringLiteral("???") ||
        text.startsWith(QStringLiteral("Lien "), Qt::CaseInsensitive)) return true;
    bool number = false;
    text.toInt(&number);
    return number;
}

DataType recoveredFieldType(const QByteArray &data, const StringRecord &record)
{
    if (record.end + 4 > data.size()) return DataType::VarChar;
    switch (little32(data, record.end)) {
    case 1: return DataType::Integer;
    case 2: return DataType::Date;
    default: return DataType::VarChar;
    }
}

QVector<Cardinality> recoveredConnectedCardinalities(const QByteArray &data)
{
    // Observed CDrawArc schema 0x27: style=15, version=2, connected=1,
    // followed by the cardinality enumeration (0=0,1; 1=1,1; 2=0,n; 3=1,n).
    static const QByteArray prefix("\x0f\0\0\0\x02\0\0\0\x01\0\0\0", 12);
    QVector<Cardinality> result;
    for (qsizetype offset = data.indexOf(prefix, StreamOffset); offset >= 0;
         offset = data.indexOf(prefix, offset + 1)) {
        if (offset + 16 > data.size()) break;
        switch (little32(data, offset + 12)) {
        case 0: result.append(Cardinality::ZeroOne); break;
        case 1: result.append(Cardinality::OneOne); break;
        case 2: result.append(Cardinality::ZeroMany); break;
        case 3: result.append(Cardinality::OneMany); break;
        default: break;
        }
    }
    return result;
}

bool recoverLargeArchive(Model &loaded, const QByteArray &data, QStringList *warnings)
{
    static const QByteArray textClassReference("\x04\x80\x40\x00\x00\x00", 6);
    QVector<qsizetype> textObjects;
    QVector<qsizetype> entityTextObjects;
    QVector<QRectF> entityRectangles;
    QVector<QRectF> associationRectangles;
    for (qsizetype offset = data.indexOf(textClassReference, StreamOffset); offset >= 0;
         offset = data.indexOf(textClassReference, offset + 1)) {
        textObjects.append(offset);
        QRectF rectangle;
        if (plausibleRectangle(data, offset, &rectangle)) {
            entityTextObjects.append(offset);
            entityRectangles.append(rectangle);
        }
        if (plausibleSizedRectangle(data, offset - 44, &rectangle))
            associationRectangles.append(rectangle);
    }
    if (entityTextObjects.size() < 2) return false;

    QStringList associationNames;
    for (qsizetype index = 0; index < entityTextObjects.size(); ++index) {
        const qsizetype begin = entityTextObjects[index];
        const qsizetype end = index + 1 < entityTextObjects.size()
            ? entityTextObjects[index + 1] : data.size();
        QSet<qsizetype> displayOffsets;
        for (const qsizetype textOffset : textObjects)
            if (textOffset >= begin && textOffset < end) displayOffsets.insert(textOffset + 0xb6);

        const auto records = archiveStringRecords(data, begin, end);
        QString entityName;
        QVector<Field> fields;
        for (const auto &record : records) {
            if (displayOffsets.contains(record.offset)) {
                if (isDisplayNoise(record.value)) continue;
                if (entityName.isEmpty()) entityName = record.value;
                else if (!associationNames.contains(record.value)) associationNames.append(record.value);
                continue;
            }
            Field field;
            field.id = newId();
            field.name = record.value;
            field.logicalName = normalizedLogicalName(record.value);
            field.type = recoveredFieldType(data, record);
            field.primaryKey = record.offset >= 4 && little32(data, record.offset - 4) != 0;
            field.notNull = field.primaryKey;
            fields.append(field);
        }
        if (entityName.isEmpty()) return false;
        loaded.entities.append({newId(), entityName, normalizedLogicalName(entityName),
                                entityRectangles[index].topLeft(), fields});
    }

    for (qsizetype i = 0; i < associationNames.size(); ++i) {
        const QPointF position = i < associationRectangles.size()
            ? associationRectangles[i].topLeft()
            : QPointF();
        loaded.associations.append({newId(), associationNames[i],
            normalizedLogicalName(associationNames[i]), position, {}});
    }

    bool spatialLinksRecovered = false;
    const auto cardinalities = recoveredConnectedCardinalities(data);
    if (associationRectangles.size() == loaded.associations.size() &&
        cardinalities.size() == loaded.associations.size() * 2 && loaded.entities.size() >= 2) {
        for (qsizetype associationIndex = 0; associationIndex < loaded.associations.size(); ++associationIndex) {
            QVector<qsizetype> entityIndexes;
            entityIndexes.reserve(loaded.entities.size());
            for (qsizetype i = 0; i < loaded.entities.size(); ++i) entityIndexes.append(i);
            const QPointF associationCenter = associationRectangles[associationIndex].center();
            std::stable_sort(entityIndexes.begin(), entityIndexes.end(), [&](qsizetype left, qsizetype right) {
                const auto leftDelta = entityRectangles[left].center() - associationCenter;
                const auto rightDelta = entityRectangles[right].center() - associationCenter;
                return QPointF::dotProduct(leftDelta, leftDelta) < QPointF::dotProduct(rightDelta, rightDelta);
            });
            entityIndexes.resize(2);
            std::sort(entityIndexes.begin(), entityIndexes.end());
            for (qsizetype branch = 0; branch < 2; ++branch) {
                loaded.links.append({newId(), loaded.entities[entityIndexes[branch]].id,
                    loaded.associations[associationIndex].id,
                    cardinalities[associationIndex * 2 + branch]});
            }
        }
        spatialLinksRecovered = true;
    }

    if (warnings) warnings->append(QStringLiteral(
        "IMPORT NATIF EN COURS : %1 entités, %2 rubriques, %3 associations et %4 liens ont été récupérés. "
        "Les positions, identifiants et cardinalités proviennent de l'archive. %5 "
        "Les propriétés UNIQUE et NOT NULL indépendantes d'un identifiant restent à vérifier.")
        .arg(loaded.entities.size())
        .arg([&loaded] { qsizetype count = 0; for (const auto &e : loaded.entities) count += e.fields.size(); return count; }())
        .arg(loaded.associations.size())
        .arg(loaded.links.size())
        .arg(spatialLinksRecovered
            ? QStringLiteral("L'appariement des branches aux deux entités les plus proches est une reconstruction spatiale : vérifiez-le avant utilisation.")
            : QStringLiteral("Les branches n'ont pas été créées lorsque leur appariement ne pouvait pas être établi sans ambiguïté.")));
    return true;
}

QString unsupportedMessage(quint16 objectCount, const QStringList &strings)
{
    return QStringLiteral(
        "Archive Looping reconnue, mais ce document contient une structure qui n'est pas encore "
        "importable sans perte (%1 objets sérialisés, %2 textes). Le fichier original n'a pas été "
        "modifié. La prise en charge des rubriques, associations et liens est en cours.")
        .arg(objectCount).arg(strings.size());
}

} // namespace

bool LoopingImporter::resemblesNativeArchive(const QByteArray &data)
{
    if (data.size() < StreamOffset + 2) return false;
    // All observed Looping 4.1 documents share this font/settings header and start
    // their MFC object stream at 0xA20. Keep both checks to avoid extension-only guesses.
    static const QByteArray utf16Calibri("C\0a\0l\0i\0b\0r\0i\0", 14);
    if (!data.left(StreamOffset).contains(utf16Calibri)) return false;
    const quint16 objectCount = little16(data, StreamOffset);
    if (objectCount == 0) return data.size() == StreamOffset + 2;
    if (data.size() < StreamOffset + 10 || objectCount > 10000) return false;
    const QByteArray stream = data.mid(StreamOffset, qMin<qsizetype>(data.size() - StreamOffset, 128));
    return stream.contains("CDrawEntite") || stream.contains("CDrawArc") ||
           stream.contains("CDrawRelation") || stream.contains("CDrawTexte");
}

bool LoopingImporter::load(Model &model, const QByteArray &data, const QString &sourceName,
                           QString *error, QStringList *warnings)
{
    if (!resemblesNativeArchive(data)) {
        if (error) *error = QStringLiteral("Ce fichier n'est pas une archive Looping native reconnue.");
        return false;
    }

    Model loaded;
    loaded.name = QFileInfo(sourceName).completeBaseName();
    const quint16 objectCount = little16(data, StreamOffset);
    if (objectCount == 0) {
        model = std::move(loaded);
        if (warnings) warnings->append(QStringLiteral(
            "Archive Looping importée en lecture seule. Utilisez Enregistrer sous pour créer un fichier .lrea."));
        return true;
    }

    const QByteArray entityClass("CDrawEntite");
    const qsizetype classOffset = data.indexOf(entityClass, StreamOffset);
    const QStringList strings = archiveStrings(data);

    // Proven subset: one CDrawEntite, its title CDrawTexte, no rubric and no
    // relation. In the observed 4.1 schema the base drawing rectangle precedes
    // CDrawTexte and begins 0xB7 bytes after the class name.
    if (objectCount == 4 && classOffset >= StreamOffset && strings.size() == 1 &&
        !data.mid(StreamOffset).contains("CDrawRelation") &&
        !data.mid(StreamOffset).contains("CDrawArc")) {
        const qsizetype rectangleOffset = classOffset + 0xb7;
        if (rectangleOffset + 16 <= data.size()) {
            const qint32 left = little32(data, rectangleOffset);
            const qint32 top = little32(data, rectangleOffset + 4);
            const qint32 right = little32(data, rectangleOffset + 8);
            const qint32 bottom = little32(data, rectangleOffset + 12);
            if (right > left && bottom > top && left >= -100000 && top >= -100000 &&
                right <= 100000 && bottom <= 100000) {
                const QString name = strings.constFirst();
                loaded.entities.append({newId(), name, normalizedLogicalName(name),
                                        QPointF(left, top), {}});
                model = std::move(loaded);
                if (warnings) warnings->append(QStringLiteral(
                    "Archive Looping importée en lecture seule. Utilisez Enregistrer sous pour créer un fichier .lrea."));
                return true;
            }
        }
    }

    if (recoverLargeArchive(loaded, data, warnings)) {
        model = std::move(loaded);
        if (warnings) warnings->append(QStringLiteral(
            "Le fichier Looping original est resté en lecture seule. Enregistrer créera un nouveau fichier .lrea."));
        return true;
    }

    if (error) *error = unsupportedMessage(objectCount, strings);
    return false;
}

} // namespace looping
