#include "GetWeatherForecast.h"
#include "SearchWeatherLocations.h"

#include <QHash>
#include <QTest>
#include <QTimeZone>

#include <utility>

using namespace stukit::weather;

namespace {

WeatherLocation location(QString name = QStringLiteral("上海"))
{
    return *WeatherLocation::create(WeatherLocationData{
        .name = std::move(name),
        .country = QStringLiteral("中国"),
        .administrativeArea = QStringLiteral("上海"),
        .timezoneId = QStringLiteral("Asia/Shanghai"),
        .latitude = 31.2304,
        .longitude = 121.4737,
    });
}

WeatherForecast forecastFor(const WeatherLocation &place)
{
    const QTimeZone zone(place.timezoneId().toUtf8());
    return *WeatherForecast::create(WeatherForecastData{
        .location = place,
        .current = CurrentWeather{
            .observedAt = QDateTime(
                QDate(2026, 10, 13), QTime(10, 0), zone),
            .temperatureCelsius = 22.0,
            .apparentTemperatureCelsius = 22.5,
            .relativeHumidityPercent = 60,
            .precipitationMillimetres = 0.0,
            .windSpeedKilometresPerHour = 8.0,
            .windDirectionDegrees = 120,
            .condition = WeatherCondition::Clear,
            .daytime = true,
        },
        .daily = {DailyWeather{
            .date = QDate(2026, 10, 13),
            .minimumTemperatureCelsius = 16.0,
            .maximumTemperatureCelsius = 24.0,
            .precipitationProbabilityPercent = 10,
            .precipitationMillimetres = 0.0,
            .condition = WeatherCondition::Clear,
            .sunrise = QDateTime(
                QDate(2026, 10, 13), QTime(5, 55), zone),
            .sunset = QDateTime(
                QDate(2026, 10, 13), QTime(17, 25), zone),
        }},
        .fetchedAtUtc = QDateTime(
            QDate(2026, 10, 13), QTime(2, 5), QTimeZone::UTC),
    });
}

class FakeLocationSearchService final : public ILocationSearchService
{
public:
    WeatherRequestId search(
        QString query,
        LocationSearchCompletion completion) override
    {
        const WeatherRequestId id = ++lastId;
        queries.insert(id, std::move(query));
        completions.insert(id, std::move(completion));
        return id;
    }

    void cancel(WeatherRequestId requestId) override
    {
        cancelled.push_back(requestId);
    }

    void complete(WeatherRequestId requestId, LocationSearchResult result)
    {
        auto completion = completions.take(requestId);
        completion(std::move(result));
    }

    WeatherRequestId lastId{0};
    QHash<WeatherRequestId, QString> queries;
    QHash<WeatherRequestId, LocationSearchCompletion> completions;
    std::vector<WeatherRequestId> cancelled;
};

class FakeForecastService final : public IWeatherForecastService
{
public:
    WeatherRequestId fetch(
        WeatherLocation place,
        ForecastCompletion completion) override
    {
        const WeatherRequestId id = ++lastId;
        locations.insert(id, std::move(place));
        completions.insert(id, std::move(completion));
        return id;
    }

    void cancel(WeatherRequestId requestId) override
    {
        cancelled.push_back(requestId);
    }

    void complete(WeatherRequestId requestId, ForecastResult result)
    {
        auto completion = completions.take(requestId);
        completion(std::move(result));
    }

    WeatherRequestId lastId{0};
    QHash<WeatherRequestId, WeatherLocation> locations;
    QHash<WeatherRequestId, ForecastCompletion> completions;
    std::vector<WeatherRequestId> cancelled;
};

} // namespace

class WeatherApplicationTest final : public QObject
{
    Q_OBJECT

private slots:
    void normalizesAndForwardsSearch();
    void rejectsInvalidSearchWithoutProviderCall();
    void ignoresStaleSearchResult();
    void forwardsForecastAndError();
    void ignoresStaleForecast();
};

void WeatherApplicationTest::normalizesAndForwardsSearch()
{
    FakeLocationSearchService service;
    SearchWeatherLocations search(service);
    std::vector<WeatherLocation> received;

    search.execute(
        QStringLiteral("  上海  "),
        [&received](LocationSearchResult result) {
            received = std::get<std::vector<WeatherLocation>>(
                std::move(result));
        });
    QCOMPARE(service.queries.value(1), QStringLiteral("上海"));

    service.complete(1, std::vector<WeatherLocation>{location()});
    QCOMPARE(received.size(), std::size_t{1});
    QCOMPARE(received.front().name(), QStringLiteral("上海"));
}

void WeatherApplicationTest::rejectsInvalidSearchWithoutProviderCall()
{
    FakeLocationSearchService service;
    SearchWeatherLocations search(service);
    WeatherServiceError error = WeatherServiceError::Cancelled;

    search.execute(QStringLiteral("a"), [&error](LocationSearchResult result) {
        error = std::get<WeatherServiceError>(result);
    });

    QCOMPARE(error, WeatherServiceError::InvalidInput);
    QCOMPARE(service.lastId, WeatherRequestId{0});
}

void WeatherApplicationTest::ignoresStaleSearchResult()
{
    FakeLocationSearchService service;
    SearchWeatherLocations search(service);
    int completionCount = 0;

    search.execute(QStringLiteral("上海"), [&completionCount](auto) {
        ++completionCount;
    });
    search.execute(QStringLiteral("北京"), [&completionCount](auto) {
        ++completionCount;
    });

    QCOMPARE(service.cancelled, std::vector<WeatherRequestId>{1});
    service.complete(1, std::vector<WeatherLocation>{location()});
    QCOMPARE(completionCount, 0);
    service.complete(
        2,
        std::vector<WeatherLocation>{location(QStringLiteral("北京"))});
    QCOMPARE(completionCount, 1);
}

void WeatherApplicationTest::forwardsForecastAndError()
{
    FakeForecastService service;
    GetWeatherForecast getForecast(service);
    const WeatherLocation place = location();
    QString receivedName;

    getForecast.execute(place, [&receivedName](ForecastResult result) {
        receivedName = std::get<WeatherForecast>(result).location().name();
    });
    QCOMPARE(service.locations.constFind(1).value().name(),
             QStringLiteral("上海"));
    service.complete(1, forecastFor(place));
    QCOMPARE(receivedName, QStringLiteral("上海"));

    WeatherServiceError error = WeatherServiceError::Cancelled;
    getForecast.execute(place, [&error](ForecastResult result) {
        error = std::get<WeatherServiceError>(result);
    });
    service.complete(2, WeatherServiceError::RateLimited);
    QCOMPARE(error, WeatherServiceError::RateLimited);
}

void WeatherApplicationTest::ignoresStaleForecast()
{
    FakeForecastService service;
    GetWeatherForecast getForecast(service);
    int completionCount = 0;

    getForecast.execute(location(), [&completionCount](auto) {
        ++completionCount;
    });
    getForecast.execute(
        location(QStringLiteral("北京")),
        [&completionCount](auto) { ++completionCount; });

    QCOMPARE(service.cancelled, std::vector<WeatherRequestId>{1});
    service.complete(1, WeatherServiceError::NetworkUnavailable);
    QCOMPARE(completionCount, 0);
    service.complete(2, WeatherServiceError::NetworkUnavailable);
    QCOMPARE(completionCount, 1);
}

QTEST_APPLESS_MAIN(WeatherApplicationTest)

#include "WeatherApplicationTest.moc"
