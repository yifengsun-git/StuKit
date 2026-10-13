#pragma once

#include "IWeatherForecastService.h"

#include <memory>

namespace stukit::weather {

class GetWeatherForecast final
{
public:
    explicit GetWeatherForecast(IWeatherForecastService &service);
    ~GetWeatherForecast();

    GetWeatherForecast(const GetWeatherForecast &) = delete;
    GetWeatherForecast &operator=(const GetWeatherForecast &) = delete;

    quint64 execute(
        WeatherLocation location,
        ForecastCompletion completion);
    void cancel();

private:
    struct State;

    IWeatherForecastService &m_service;
    std::shared_ptr<State> m_state;
};

} // namespace stukit::weather
