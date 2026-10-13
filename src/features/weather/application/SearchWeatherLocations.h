#pragma once

#include "ILocationSearchService.h"

#include <memory>

namespace stukit::weather {

class SearchWeatherLocations final
{
public:
    static constexpr qsizetype MinimumQueryLength = 2;
    static constexpr qsizetype MaximumQueryLength = 100;

    explicit SearchWeatherLocations(ILocationSearchService &service);
    ~SearchWeatherLocations();

    SearchWeatherLocations(const SearchWeatherLocations &) = delete;
    SearchWeatherLocations &operator=(const SearchWeatherLocations &) = delete;

    quint64 execute(QString query, LocationSearchCompletion completion);
    void cancel();

private:
    struct State;

    ILocationSearchService &m_service;
    std::shared_ptr<State> m_state;
};

} // namespace stukit::weather
