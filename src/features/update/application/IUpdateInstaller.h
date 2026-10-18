#pragma once

#include <QString>

namespace stukit::update {

class IUpdateInstaller
{
public:
    virtual ~IUpdateInstaller() = default;

    [[nodiscard]] virtual bool supported() const noexcept = 0;
    [[nodiscard]] virtual bool startInstaller(
        const QString &packagePath,
        QString &errorMessage) = 0;
};

} // namespace stukit::update
