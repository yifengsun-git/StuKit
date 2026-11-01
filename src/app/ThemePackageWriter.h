#pragma once

#include <QString>

namespace stukit::app
{

class ThemePackageWriter final
{
public:
    [[nodiscard]] static bool write(const QString &themeDirectory, const QString &targetPath,
                                    QString &errorMessage);
};

} // namespace stukit::app
