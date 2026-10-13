#include "WeatherCondition.h"
#include "WeatherForecast.h"
#include "WeatherLocation.h"

#include <QTest>
#include <QTimeZone>

#include <limits>

using namespace stukit::weather;

namespace {

WeatherLocation validLocation()
{
    return *WeatherLocation::create(WeatherLocationData{
        .name = QStringLiteral("上海"),
        .country = QStringLiteral("中国"),
        .administrativeArea = QStringLiteral("上海"),
        .timezoneId = QStringLiteral("Asia/Shanghai"),
        .latitude = 31.2304,
        .longitude = 121.4737,
    });
}

WeatherForecastData validForecastData()
{
    const QTimeZone zone("Asia/Shanghai");
    return WeatherForecastData{
        .location = validLocation(),
        .current = CurrentWeather{
            .observedAt = QDateTime(
                QDate(2026, 10, 13), QTime(10, 0), zone),
            .temperatureCelsius = 23.5,
            .apparentTemperatureCelsius = 24.0,
            .relativeHumidityPercent = 68,
            .precipitationMillimetres = 0.0,
            .windSpeedKilometresPerHour = 12.0,
            .windDirectionDegrees = 90,
            .condition = WeatherCondition::PartlyCloudy,
            .daytime = true,
        },
        .daily = {
            DailyWeather{
                .date = QDate(2026, 10, 13),
                .minimumTemperatureCelsius = 18.0,
                .maximumTemperatureCelsius = 25.0,
                .precipitationProbabilityPercent = 20,
                .precipitationMillimetres = 0.2,
                .condition = WeatherCondition::PartlyCloudy,
                .sunrise = QDateTime(
                    QDate(2026, 10, 13), QTime(5, 55), zone),
                .sunset = QDateTime(
                    QDate(2026, 10, 13), QTime(17, 25), zone),
            },
            DailyWeather{
                .date = QDate(2026, 10, 14),
                .minimumTemperatureCelsius = 17.0,
                .maximumTemperatureCelsius = 24.0,
                .precipitationProbabilityPercent = 45,
                .precipitationMillimetres = 2.0,
                .condition = WeatherCondition::Rain,
                .sunrise = QDateTime(
                    QDate(2026, 10, 14), QTime(5, 56), zone),
                .sunset = QDateTime(
                    QDate(2026, 10, 14), QTime(17, 24), zone),
            },
        },
        .fetchedAtUtc = QDateTime(
            QDate(2026, 10, 13), QTime(2, 5), QTimeZone::UTC),
    };
}

} // namespace

class WeatherDomainTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndNormalizesLocation();
    void rejectsInvalidLocation_data();
    void rejectsInvalidLocation();
    void mapsWmoWeatherCodes();
    void createsForecast();
    void rejectsInvalidForecast_data();
    void rejectsInvalidForecast();
};

void WeatherDomainTest::createsAndNormalizesLocation()
{
    auto location = WeatherLocation::create(WeatherLocationData{
        .name = QStringLiteral("  上海  "),
        .country = QStringLiteral(" 中国 "),
        .administrativeArea = QStringLiteral(" 上海 "),
        .timezoneId = QStringLiteral(" Asia/Shanghai "),
        .latitude = 31.2304,
        .longitude = 121.4737,
    });

    QVERIFY(location.has_value());
    QCOMPARE(location->name(), QStringLiteral("上海"));
    QCOMPARE(location->country(), QStringLiteral("中国"));
    QCOMPARE(location->timezoneId(), QStringLiteral("Asia/Shanghai"));
    QCOMPARE(location->latitude(), 31.2304);
}

void WeatherDomainTest::rejectsInvalidLocation_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("timezone");
    QTest::addColumn<double>("latitude");
    QTest::addColumn<double>("longitude");

    QTest::newRow("empty name")
        << QString{} << QStringLiteral("Asia/Shanghai") << 31.0 << 121.0;
    QTest::newRow("invalid timezone")
        << QStringLiteral("上海") << QStringLiteral("Mars/Base")
        << 31.0 << 121.0;
    QTest::newRow("latitude outside range")
        << QStringLiteral("上海") << QStringLiteral("Asia/Shanghai")
        << 91.0 << 121.0;
    QTest::newRow("longitude outside range")
        << QStringLiteral("上海") << QStringLiteral("Asia/Shanghai")
        << 31.0 << -181.0;
    QTest::newRow("not finite")
        << QStringLiteral("上海") << QStringLiteral("Asia/Shanghai")
        << std::numeric_limits<double>::quiet_NaN() << 121.0;
}

void WeatherDomainTest::rejectsInvalidLocation()
{
    QFETCH(QString, name);
    QFETCH(QString, timezone);
    QFETCH(double, latitude);
    QFETCH(double, longitude);

    QVERIFY(!WeatherLocation::create(WeatherLocationData{
        .name = name,
        .timezoneId = timezone,
        .latitude = latitude,
        .longitude = longitude,
    }).has_value());
}

void WeatherDomainTest::mapsWmoWeatherCodes()
{
    QCOMPARE(conditionFromWmoCode(0), WeatherCondition::Clear);
    QCOMPARE(conditionFromWmoCode(3), WeatherCondition::Overcast);
    QCOMPARE(conditionFromWmoCode(63), WeatherCondition::Rain);
    QCOMPARE(conditionFromWmoCode(85), WeatherCondition::SnowShowers);
    QCOMPARE(conditionFromWmoCode(95), WeatherCondition::Thunderstorm);
    QCOMPARE(conditionFromWmoCode(42), WeatherCondition::Unknown);
}

void WeatherDomainTest::createsForecast()
{
    const auto forecast = WeatherForecast::create(validForecastData());

    QVERIFY(forecast.has_value());
    QCOMPARE(forecast->location().name(), QStringLiteral("上海"));
    QCOMPARE(forecast->current().relativeHumidityPercent, 68);
    QCOMPARE(forecast->daily().size(), std::size_t{2});
    QCOMPARE(forecast->daily().back().condition, WeatherCondition::Rain);
}

void WeatherDomainTest::rejectsInvalidForecast_data()
{
    QTest::addColumn<int>("caseNumber");

    QTest::newRow("invalid humidity") << 0;
    QTest::newRow("minimum above maximum") << 1;
    QTest::newRow("unsorted days") << 2;
    QTest::newRow("duplicate day") << 3;
    QTest::newRow("no daily forecast") << 4;
    QTest::newRow("fetch time is not UTC") << 5;
}

void WeatherDomainTest::rejectsInvalidForecast()
{
    QFETCH(int, caseNumber);
    auto data = validForecastData();
    switch (caseNumber) {
    case 0:
        data.current.relativeHumidityPercent = 101;
        break;
    case 1:
        data.daily.front().minimumTemperatureCelsius = 30.0;
        break;
    case 2:
        std::swap(data.daily[0], data.daily[1]);
        break;
    case 3:
        data.daily[1].date = data.daily[0].date;
        break;
    case 4:
        data.daily.clear();
        break;
    case 5:
        data.fetchedAtUtc = QDateTime(
            QDate(2026, 10, 13), QTime(10, 5),
            QTimeZone("Asia/Shanghai"));
        break;
    }

    QVERIFY(!WeatherForecast::create(std::move(data)).has_value());
}

QTEST_APPLESS_MAIN(WeatherDomainTest)

#include "WeatherDomainTest.moc"
