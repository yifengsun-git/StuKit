#pragma once

#include "WeatherLocation.h"

#include <QString>
#include <QUrl>

namespace stukit::weather {

class OpenMeteoUrlBuilder final
{
public:
    [[nodiscard]] static QUrl locationSearch(QString query);
    [[nodiscard]] static QUrl forecast(const WeatherLocation &location);
};

} // namespace stukit::weather
