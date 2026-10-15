#include "QWeatherWeatherService.h"

#include "QWeatherJsonParser.h"
#include "QWeatherUrlBuilder.h"

#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

#include <optional>
#include <utility>

namespace stukit::weather {
namespace {

std::optional<WeatherServiceError> replyError(QNetworkReply &reply)
{
    const int status = reply.attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply.error() == QNetworkReply::OperationCanceledError) {
        return WeatherServiceError::Cancelled;
    }
    if (status == 401 || status == 403) {
        return WeatherServiceError::AuthenticationFailed;
    }
    if (status == 429) {
        return WeatherServiceError::RateLimited;
    }
    if (status >= 500) {
        return WeatherServiceError::ServiceUnavailable;
    }
    if (reply.error() != QNetworkReply::NoError || status < 200 || status >= 300) {
        return WeatherServiceError::NetworkUnavailable;
    }
    return std::nullopt;
}

} // namespace

QWeatherWeatherService::QWeatherWeatherService(
    QNetworkAccessManager &network,
    QString apiHost,
    QByteArray apiKey,
    QObject *parent)
    : QObject(parent)
    , m_network(network)
    , m_apiHost(std::move(apiHost))
    , m_apiKey(std::move(apiKey))
{
    m_apiHost = m_apiHost.trimmed();
    m_apiKey = m_apiKey.trimmed();
}

WeatherRequestId QWeatherWeatherService::search(
    QString query,
    LocationSearchCompletion completion)
{
    const WeatherRequestId requestId = nextRequestId();
    if (!hasConfiguration()) {
        QTimer::singleShot(0, this, [completion = std::move(completion)]() mutable {
            if (completion) {
                completion(WeatherServiceError::ConfigurationMissing);
            }
        });
        return requestId;
    }

    QNetworkReply *reply = m_network.get(requestFor(
        QWeatherUrlBuilder::locationSearch(m_apiHost, std::move(query))));
    m_searches.insert(requestId, PendingSearch{reply, std::move(completion)});
    connect(reply, &QNetworkReply::finished, this, [this, requestId] {
        finishSearch(requestId);
    });
    return requestId;
}

WeatherRequestId QWeatherWeatherService::fetch(
    WeatherLocation location,
    ForecastCompletion completion)
{
    const WeatherRequestId requestId = nextRequestId();
    if (!hasConfiguration()) {
        QTimer::singleShot(0, this, [completion = std::move(completion)]() mutable {
            if (completion) {
                completion(WeatherServiceError::ConfigurationMissing);
            }
        });
        return requestId;
    }

    QNetworkReply *currentReply = m_network.get(requestFor(
        QWeatherUrlBuilder::currentWeather(m_apiHost, location)));
    QNetworkReply *dailyReply = m_network.get(requestFor(
        QWeatherUrlBuilder::dailyForecast(m_apiHost, location)));
    m_forecasts.insert(
        requestId,
        PendingForecast{
            .currentReply = currentReply,
            .dailyReply = dailyReply,
            .location = std::move(location),
            .completion = std::move(completion),
        });
    connect(currentReply, &QNetworkReply::finished, this, [this, requestId] {
        finishForecastPart(requestId, ForecastPart::Current);
    });
    connect(dailyReply, &QNetworkReply::finished, this, [this, requestId] {
        finishForecastPart(requestId, ForecastPart::Daily);
    });
    return requestId;
}

void QWeatherWeatherService::cancel(WeatherRequestId requestId)
{
    if (const auto search = m_searches.constFind(requestId);
        search != m_searches.cend() && search->reply != nullptr) {
        search->reply->abort();
        return;
    }
    if (const auto forecast = m_forecasts.constFind(requestId);
        forecast != m_forecasts.cend()) {
        if (forecast->currentReply != nullptr) {
            forecast->currentReply->abort();
        }
        if (forecast->dailyReply != nullptr) {
            forecast->dailyReply->abort();
        }
    }
}

WeatherRequestId QWeatherWeatherService::nextRequestId()
{
    ++m_lastRequestId;
    if (m_lastRequestId == InvalidWeatherRequestId) {
        ++m_lastRequestId;
    }
    return m_lastRequestId;
}

QNetworkRequest QWeatherWeatherService::requestFor(const QUrl &url) const
{
    QNetworkRequest request(url);
    request.setRawHeader("X-QW-Api-Key", m_apiKey);
    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        QStringLiteral("StuKit/0.1 (personal weather client)"));
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(15000);
    return request;
}

bool QWeatherWeatherService::hasConfiguration() const
{
    return !m_apiKey.isEmpty()
        && QWeatherUrlBuilder::isValidApiHost(m_apiHost);
}

void QWeatherWeatherService::finishSearch(WeatherRequestId requestId)
{
    PendingSearch pending = m_searches.take(requestId);
    if (pending.reply == nullptr || !pending.completion) {
        return;
    }
    QNetworkReply *reply = pending.reply;
    const QByteArray body = reply->readAll();
    const auto error = replyError(*reply);
    reply->deleteLater();
    if (error.has_value()) {
        pending.completion(*error);
        return;
    }
    pending.completion(QWeatherJsonParser::parseLocations(body));
}

void QWeatherWeatherService::finishForecastPart(
    WeatherRequestId requestId,
    ForecastPart part)
{
    auto found = m_forecasts.find(requestId);
    if (found == m_forecasts.end()) {
        return;
    }

    QPointer<QNetworkReply> &replyPointer = part == ForecastPart::Current
        ? found->currentReply : found->dailyReply;
    if (replyPointer == nullptr) {
        return;
    }
    QNetworkReply *reply = replyPointer;
    const QByteArray body = reply->readAll();
    const auto error = replyError(*reply);
    replyPointer = nullptr;
    reply->deleteLater();
    if (error.has_value()) {
        failForecast(requestId, *error);
        return;
    }

    if (part == ForecastPart::Current) {
        found->currentJson = body;
        found->currentFinished = true;
    } else {
        found->dailyJson = body;
        found->dailyFinished = true;
    }
    if (!found->currentFinished || !found->dailyFinished) {
        return;
    }

    PendingForecast pending = std::move(found.value());
    m_forecasts.erase(found);
    if (pending.completion) {
        pending.completion(QWeatherJsonParser::parseForecast(
            pending.currentJson,
            pending.dailyJson,
            pending.location,
            QDateTime::currentDateTimeUtc()));
    }
}

void QWeatherWeatherService::failForecast(
    WeatherRequestId requestId,
    WeatherServiceError error)
{
    auto found = m_forecasts.find(requestId);
    if (found == m_forecasts.end()) {
        return;
    }
    PendingForecast pending = std::move(found.value());
    m_forecasts.erase(found);
    if (pending.currentReply != nullptr) {
        pending.currentReply->abort();
    }
    if (pending.dailyReply != nullptr) {
        pending.dailyReply->abort();
    }
    if (pending.completion) {
        pending.completion(error);
    }
}

} // namespace stukit::weather
