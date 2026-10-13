#include "OpenMeteoWeatherService.h"

#include "OpenMeteoJsonParser.h"
#include "OpenMeteoUrlBuilder.h"

#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <utility>

namespace stukit::weather {
namespace {

std::optional<WeatherServiceError> replyError(QNetworkReply &reply)
{
    const int status = reply.attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 429) {
        return WeatherServiceError::RateLimited;
    }
    if (status >= 500) {
        return WeatherServiceError::ServiceUnavailable;
    }
    if (reply.error() == QNetworkReply::OperationCanceledError) {
        return WeatherServiceError::Cancelled;
    }
    if (reply.error() != QNetworkReply::NoError || status < 200 || status >= 300) {
        return WeatherServiceError::NetworkUnavailable;
    }
    return std::nullopt;
}

QNetworkRequest requestFor(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        QStringLiteral("StuKit/0.1 (personal weather client)"));
    request.setTransferTimeout(15000);
    return request;
}

} // namespace

OpenMeteoWeatherService::OpenMeteoWeatherService(
    QNetworkAccessManager &network,
    QObject *parent)
    : QObject(parent)
    , m_network(network)
{
}

WeatherRequestId OpenMeteoWeatherService::search(
    QString query,
    LocationSearchCompletion completion)
{
    const WeatherRequestId requestId = nextRequestId();
    QNetworkReply *reply = m_network.get(
        requestFor(OpenMeteoUrlBuilder::locationSearch(std::move(query))));
    m_searches.insert(requestId, PendingSearch{reply, std::move(completion)});
    connect(reply, &QNetworkReply::finished, this, [this, requestId] {
        finishSearch(requestId);
    });
    return requestId;
}

WeatherRequestId OpenMeteoWeatherService::fetch(
    WeatherLocation location,
    ForecastCompletion completion)
{
    const WeatherRequestId requestId = nextRequestId();
    QNetworkReply *reply = m_network.get(
        requestFor(OpenMeteoUrlBuilder::forecast(location)));
    m_forecasts.insert(
        requestId,
        PendingForecast{reply, std::move(location), std::move(completion)});
    connect(reply, &QNetworkReply::finished, this, [this, requestId] {
        finishForecast(requestId);
    });
    return requestId;
}

void OpenMeteoWeatherService::cancel(WeatherRequestId requestId)
{
    if (const auto search = m_searches.constFind(requestId);
        search != m_searches.cend() && search->reply != nullptr) {
        search->reply->abort();
        return;
    }
    if (const auto forecast = m_forecasts.constFind(requestId);
        forecast != m_forecasts.cend() && forecast->reply != nullptr) {
        forecast->reply->abort();
    }
}

WeatherRequestId OpenMeteoWeatherService::nextRequestId()
{
    ++m_lastRequestId;
    if (m_lastRequestId == InvalidWeatherRequestId) {
        ++m_lastRequestId;
    }
    return m_lastRequestId;
}

void OpenMeteoWeatherService::finishSearch(WeatherRequestId requestId)
{
    PendingSearch pending = m_searches.take(requestId);
    if (pending.reply == nullptr || !pending.completion) {
        return;
    }
    QNetworkReply *reply = pending.reply;
    reply->deleteLater();
    if (const auto error = replyError(*reply)) {
        pending.completion(*error);
        return;
    }
    pending.completion(OpenMeteoJsonParser::parseLocations(reply->readAll()));
}

void OpenMeteoWeatherService::finishForecast(WeatherRequestId requestId)
{
    auto found = m_forecasts.find(requestId);
    if (found == m_forecasts.end()) {
        return;
    }
    PendingForecast pending = std::move(found.value());
    m_forecasts.erase(found);
    if (pending.reply == nullptr || !pending.completion) {
        return;
    }
    QNetworkReply *reply = pending.reply;
    reply->deleteLater();
    if (const auto error = replyError(*reply)) {
        pending.completion(*error);
        return;
    }
    pending.completion(OpenMeteoJsonParser::parseForecast(
        reply->readAll(), pending.location, QDateTime::currentDateTimeUtc()));
}

} // namespace stukit::weather
