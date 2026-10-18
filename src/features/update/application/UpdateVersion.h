#pragma once

#include <QString>

namespace stukit::update {

class UpdateVersion final
{
public:
    [[nodiscard]] static QString normalized(const QString &version);
    [[nodiscard]] static bool isNewer(
        const QString &candidate,
        const QString &current);
};

} // namespace stukit::update
