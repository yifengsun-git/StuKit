#pragma once

#include <QString>

namespace stukit::update {

class IUpdatePreparation
{
public:
    virtual ~IUpdatePreparation() = default;

    [[nodiscard]] virtual bool prepare(
        const QString &targetVersion,
        QString &backupPath,
        QString &errorMessage) = 0;
};

} // namespace stukit::update
