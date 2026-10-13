#pragma once

#include "WeatherServiceTypes.h"

#include <QString>

namespace stukit::weather {

class ILocationSearchService
{
public:
    virtual ~ILocationSearchService() = default;

    // Implementations must invoke completion asynchronously at most once.
    virtual WeatherRequestId search(
        QString query,
        LocationSearchCompletion completion) = 0;
    virtual void cancel(WeatherRequestId requestId) = 0;
};

} // namespace stukit::weather
