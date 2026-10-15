#include "QWeatherUrlBuilder.h"

#include <QUrlQuery>

namespace stukit::weather {
namespace {

QUrl baseUrl(QString apiHost)
{
    apiHost = apiHost.trimmed();
    if (!apiHost.contains(QStringLiteral("://"))) {
        apiHost.prepend(QStringLiteral("https://"));
    }

    QUrl url(apiHost);
    if (!url.isValid() || url.scheme() != QStringLiteral("https")
        || url.host().isEmpty() || !url.userInfo().isEmpty()) {
        return {};
    }
    url.setPath(QString());
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}

QString coordinate(double value)
{
    return QString::number(value, 'f', 2);
}

QUrl weatherUrl(
    const QString &apiHost,
    const QString &resource,
    const WeatherLocation &location)
{
    QUrl url = baseUrl(apiHost);
    if (!url.isValid()) {
        return {};
    }
    url.setPath(QStringLiteral("/weather/v1/%1/%2/%3")
                    .arg(resource,
                         coordinate(location.latitude()),
                         coordinate(location.longitude())));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("localTime"), QStringLiteral("true"));
    parameters.addQueryItem(QStringLiteral("lang"), QStringLiteral("zh"));
    url.setQuery(parameters);
    return url;
}

} // namespace

QUrl QWeatherUrlBuilder::locationSearch(
    const QString &apiHost,
    QString query)
{
    QUrl url = baseUrl(apiHost);
    if (!url.isValid()) {
        return {};
    }
    url.setPath(QStringLiteral("/geo/v2/city/lookup"));
    QUrlQuery parameters;
    parameters.addQueryItem(QStringLiteral("location"), query.trimmed());
    parameters.addQueryItem(QStringLiteral("number"), QStringLiteral("8"));
    parameters.addQueryItem(QStringLiteral("lang"), QStringLiteral("zh"));
    url.setQuery(parameters);
    return url;
}

QUrl QWeatherUrlBuilder::currentWeather(
    const QString &apiHost,
    const WeatherLocation &location)
{
    return weatherUrl(apiHost, QStringLiteral("current"), location);
}

QUrl QWeatherUrlBuilder::dailyForecast(
    const QString &apiHost,
    const WeatherLocation &location)
{
    QUrl url = weatherUrl(apiHost, QStringLiteral("daily"), location);
    if (!url.isValid()) {
        return {};
    }
    QUrlQuery parameters(url);
    parameters.addQueryItem(QStringLiteral("days"), QStringLiteral("7"));
    url.setQuery(parameters);
    return url;
}

bool QWeatherUrlBuilder::isValidApiHost(const QString &apiHost)
{
    return baseUrl(apiHost).isValid();
}

} // namespace stukit::weather
