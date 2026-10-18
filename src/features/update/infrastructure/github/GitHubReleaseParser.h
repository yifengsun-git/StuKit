#pragma once

#include "IUpdateService.h"

#include <QByteArray>

namespace stukit::update {

class GitHubReleaseParser final
{
public:
    [[nodiscard]] static UpdateCheckResult parse(const QByteArray &json);
};

} // namespace stukit::update
