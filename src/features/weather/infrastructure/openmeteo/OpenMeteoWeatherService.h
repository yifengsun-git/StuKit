#pragma once

#include "ILocationSearchService.h"
#include "IWeatherForecastService.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QPointer>

namespace stukit::weather {

class OpenMeteoWeatherService final : public QObject,
                                      public ILocationSearchService,
                                      public IWeatherForecastService
{
public:
    explicit OpenMeteoWeatherService(
        QNetworkAccessManager &network,
        QObject *parent = nullptr);

    WeatherRequestId search(
        QString query,
        LocationSearchCompletion completion) override;
    WeatherRequestId fetch(
        WeatherLocation location,
        ForecastCompletion completion) override;
    void cancel(WeatherRequestId requestId) override;

private:
    struct PendingSearch
    {
        QPointer<QNetworkReply> reply;
        LocationSearchCompletion completion;
    };

    struct PendingForecast
    {
        QPointer<QNetworkReply> reply;
        WeatherLocation location;
        ForecastCompletion completion;
    };

    [[nodiscard]] WeatherRequestId nextRequestId();
    void finishSearch(WeatherRequestId requestId);
    void finishForecast(WeatherRequestId requestId);

    QNetworkAccessManager &m_network;
    WeatherRequestId m_lastRequestId{InvalidWeatherRequestId};
    QHash<WeatherRequestId, PendingSearch> m_searches;
    QHash<WeatherRequestId, PendingForecast> m_forecasts;
};

} // namespace stukit::weather
