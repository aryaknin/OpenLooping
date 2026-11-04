#pragma once

#include "core/model.h"

namespace looping {

struct GeneratedOutput {
    QString mld;
    QString sql;
    QStringList warnings;
};

class RelationalGenerator {
public:
    static GeneratedOutput generate(const Model &model);
};

} // namespace looping
