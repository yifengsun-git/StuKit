#pragma once

#include "IUpdateInstaller.h"

namespace stukit::update {

class WindowsUpdateInstaller final : public IUpdateInstaller
{
public:
    [[nodiscard]] bool supported() const noexcept override;
    [[nodiscard]] bool startInstaller(
        const QString &packagePath,
        QString &errorMessage) override;
};

} // namespace stukit::update
