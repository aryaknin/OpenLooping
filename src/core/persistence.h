#pragma once

#include "core/model.h"

#include <QStringList>

namespace looping {

class Persistence {
public:
    static bool save(const Model &model, const QString &path, QString *error = nullptr);
    static bool load(Model &model, const QString &path, QString *error = nullptr,
                     QStringList *warnings = nullptr);
    static bool isNativeLoopingFile(const QString &path);
};

} // namespace looping
