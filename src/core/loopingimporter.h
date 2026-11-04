#pragma once

#include "core/model.h"

#include <QByteArray>
#include <QString>
#include <QStringList>

namespace looping {

class LoopingImporter {
public:
    static bool resemblesNativeArchive(const QByteArray &data);
    static bool load(Model &model, const QByteArray &data, const QString &sourceName,
                     QString *error = nullptr, QStringList *warnings = nullptr);
};

} // namespace looping
