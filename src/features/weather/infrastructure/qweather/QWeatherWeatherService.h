#pragma once

#include "ILocationSearchService.h"
#include "IWeatherForecastService.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QPointer>

namespace stukit::weather {

class QWeatherWeatherService final : public QObject,
                                     public ILocationSearchService,
                                     public IWeatherForecastService
{
public:
    QWeatherWeatherService(
        QNetworkAccessManager &network,
        QString apiHost,
        QByteArray apiKey,
        QObject *parent = nullptr);

    WeatherRequestId search(
        QString query,
        LocationSearchCompletion completion) override;
    WeatherRequestId fetch(
        WeatherLocation location,
        ForecastCompletion completion) override;
    void cancel(WeatherRequestId requestId) override;

private:
    enum class ForecastPart { Current, Daily };

    struct PendingSearch
    {
        QPointer<QNetworkReply> reply;
        LocationSearchCompletion completion;
    };

    struct PendingForecast
    {
        QPointer<QNetworkReply> currentReply;
        QPointer<QNetworkReply> dailyReply;
        QByteArray currentJson;
        QByteArray dailyJson;
        WeatherLocation location;
        ForecastCompletion completion;
        bool currentFinished{false};
        bool dailyFinished{false};
    };

    [[nodiscard]] WeatherRequestId nextRequestId();
    [[nodiscard]] QNetworkRequest requestFor(const QUrl &url) const;
    [[nodiscard]] bool hasConfiguration() const;
    void finishSearch(WeatherRequestId requestId);
    void finishForecastPart(
        WeatherRequestId requestId,
        ForecastPart part);
    void failForecast(
        WeatherRequestId requestId,
        WeatherServiceError error);

    QNetworkAccessManager &m_network;
    QString m_apiHost;
    QByteArray m_apiKey;
    WeatherRequestId m_lastRequestId{InvalidWeatherRequestId};
    QHash<WeatherRequestId, PendingSearch> m_searches;
    QHash<WeatherRequestId, PendingForecast> m_forecasts;
};

} // namespace stukit::weather
