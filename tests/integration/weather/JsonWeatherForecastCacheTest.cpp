#include "JsonWeatherForecastCache.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
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
    const QDate date(2026, 10, 13);
    const QTimeZone zone(place.timezoneId().toUtf8());
    return *WeatherForecast::create(WeatherForecastData{
        .location = place,
        .current = CurrentWeather{
            .observedAt = QDateTime(date, QTime(10, 0), zone),
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
            .date = date,
            .minimumTemperatureCelsius = 16.0,
            .maximumTemperatureCelsius = 24.0,
            .precipitationProbabilityPercent = 10,
            .precipitationMillimetres = 0.0,
            .condition = WeatherCondition::Clear,
            .sunrise = QDateTime(date, QTime(5, 55), zone),
            .sunset = QDateTime(date, QTime(17, 25), zone),
        }},
        .fetchedAtUtc = QDateTime(date, QTime(2, 5), QTimeZone::UTC),
    });
}

} // namespace

class JsonWeatherForecastCacheTest final : public QObject
{
    Q_OBJECT

private slots:
    void roundTripsValidForecast();
    void rejectsCorruptAndMismatchedData();
};

void JsonWeatherForecastCacheTest::roundTripsValidForecast()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral("weather-cache-test-XXXXXX")));
    QVERIFY(directory.isValid());
    JsonWeatherForecastCache cache(
        directory.filePath(QStringLiteral("forecast.json")));
    const WeatherLocation place = location();

    QVERIFY(cache.save(forecastFor(place)));
    const auto restored = cache.load(place);

    QVERIFY(restored.has_value());
    QCOMPARE(restored->location().name(), QStringLiteral("上海"));
    QCOMPARE(restored->current().temperatureCelsius, 22.0);
    QCOMPARE(restored->daily().size(), std::size_t{1});
    QCOMPARE(restored->fetchedAtUtc(),
             QDateTime(QDate(2026, 10, 13), QTime(2, 5), QTimeZone::UTC));
}

void JsonWeatherForecastCacheTest::rejectsCorruptAndMismatchedData()
{
    QTemporaryDir directory(QDir(QCoreApplication::applicationDirPath())
                                .filePath(QStringLiteral("weather-cache-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("forecast.json"));
    JsonWeatherForecastCache cache(path);

    QFile corrupt(path);
    QVERIFY(corrupt.open(QIODevice::WriteOnly));
    QCOMPARE(corrupt.write("not-json"), qint64{8});
    corrupt.close();
    QVERIFY(!cache.load(location()).has_value());

    QVERIFY(cache.save(forecastFor(location())));
    QVERIFY(!cache.load(location(QStringLiteral("杭州"))).has_value());
}

QTEST_APPLESS_MAIN(JsonWeatherForecastCacheTest)

#include "JsonWeatherForecastCacheTest.moc"
