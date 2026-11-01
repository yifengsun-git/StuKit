#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>

namespace stukit::app
{

struct ThemePackageData
{
    QByteArray themeDefinition;
    QMap<QString, QByteArray> resources;
    bool archive{false};
};

class ThemePackageReader final
{
public:
    [[nodiscard]] static bool read(const QString &filePath, ThemePackageData &package,
                                   QString &errorMessage);
};

} // namespace stukit::app
