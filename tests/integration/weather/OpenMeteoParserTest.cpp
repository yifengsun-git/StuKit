#include "OpenMeteoJsonParser.h"
#include "OpenMeteoUrlBuilder.h"

#include <QTest>
#include <QTimeZone>
#include <QUrlQuery>

using namespace stukit::weather;

namespace {

WeatherLocation shanghai()
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

const QByteArray locationJson = R"json({
  "results": [{
    "name": "上海",
    "latitude": 31.22222,
    "longitude": 121.45806,
    "timezone": "Asia/Shanghai",
    "country": "中国",
    "admin1": "上海"
  }]
})json";

const QByteArray forecastJson = R"json({
  "current": {
    "time": "2026-10-13T10:00",
    "temperature_2m": 23.5,
    "apparent_temperature": 24.1,
    "relative_humidity_2m": 68,
    "precipitation": 0.0,
    "weather_code": 2,
    "wind_speed_10m": 12.3,
    "wind_direction_10m": 90,
    "is_day": 1
  },
  "daily": {
    "time": ["2026-10-13", "2026-10-14"],
    "weather_code": [2, 63],
    "temperature_2m_max": [25.0, 23.0],
    "temperature_2m_min": [18.0, 17.0],
    "precipitation_probability_max": [20, 70],
    "precipitation_sum": [0.2, 5.6],
    "sunrise": ["2026-10-13T05:55", "2026-10-14T05:56"],
    "sunset": ["2026-10-13T17:25", "2026-10-14T17:24"]
  }
})json";

} // namespace

class OpenMeteoParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void buildsLocationUrl();
    void buildsForecastUrl();
    void parsesLocations();
    void parsesEmptyLocations();
    void parsesForecastInLocationTimeZone();
    void rejectsMalformedOrMismatchedJson();
};

void OpenMeteoParserTest::buildsLocationUrl()
{
    const QUrl url = OpenMeteoUrlBuilder::locationSearch(
        QStringLiteral(" 上海 "));
    const QUrlQuery query(url);
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), QStringLiteral("geocoding-api.open-meteo.com"));
    QCOMPARE(query.queryItemValue(QStringLiteral("name")),
             QStringLiteral("上海"));
    QCOMPARE(query.queryItemValue(QStringLiteral("language")),
             QStringLiteral("zh"));
}

void OpenMeteoParserTest::buildsForecastUrl()
{
    const QUrlQuery query(OpenMeteoUrlBuilder::forecast(shanghai()));
    QCOMPARE(query.queryItemValue(QStringLiteral("timezone")),
             QStringLiteral("Asia/Shanghai"));
    QCOMPARE(query.queryItemValue(QStringLiteral("forecast_days")),
             QStringLiteral("7"));
    QVERIFY(query.queryItemValue(QStringLiteral("current"))
                .contains(QStringLiteral("temperature_2m")));
    QVERIFY(query.queryItemValue(QStringLiteral("daily"))
                .contains(QStringLiteral("sunrise")));
}

void OpenMeteoParserTest::parsesLocations()
{
    const LocationSearchResult result =
        OpenMeteoJsonParser::parseLocations(locationJson);
    QVERIFY(std::holds_alternative<std::vector<WeatherLocation>>(result));
    const auto &locations = std::get<std::vector<WeatherLocation>>(result);
    QCOMPARE(locations.size(), std::size_t{1});
    QCOMPARE(locations.front().name(), QStringLiteral("上海"));
    QCOMPARE(locations.front().timezoneId(), QStringLiteral("Asia/Shanghai"));
}

void OpenMeteoParserTest::parsesEmptyLocations()
{
    const LocationSearchResult result =
        OpenMeteoJsonParser::parseLocations(QByteArrayLiteral("{}"));
    QVERIFY(std::holds_alternative<std::vector<WeatherLocation>>(result));
    QVERIFY(std::get<std::vector<WeatherLocation>>(result).empty());
}

void OpenMeteoParserTest::parsesForecastInLocationTimeZone()
{
    const ForecastResult result = OpenMeteoJsonParser::parseForecast(
        forecastJson,
        shanghai(),
        QDateTime(QDate(2026, 10, 13), QTime(2, 5), QTimeZone::UTC));
    QVERIFY(std::holds_alternative<WeatherForecast>(result));
    const WeatherForecast &forecast = std::get<WeatherForecast>(result);
    QCOMPARE(forecast.current().condition, WeatherCondition::PartlyCloudy);
    QCOMPARE(forecast.current().observedAt.timeZone().id(),
             QByteArray("Asia/Shanghai"));
    QCOMPARE(forecast.daily().size(), std::size_t{2});
    QCOMPARE(forecast.daily().back().condition, WeatherCondition::Rain);
    QCOMPARE(forecast.daily().back().precipitationProbabilityPercent, 70);
}

void OpenMeteoParserTest::rejectsMalformedOrMismatchedJson()
{
    QCOMPARE(
        std::get<WeatherServiceError>(
            OpenMeteoJsonParser::parseLocations(QByteArrayLiteral("not-json"))),
        WeatherServiceError::InvalidResponse);

    QByteArray mismatched = forecastJson;
    mismatched.replace(
        "\"weather_code\": [2, 63]",
        "\"weather_code\": [2]");
    QCOMPARE(
        std::get<WeatherServiceError>(OpenMeteoJsonParser::parseForecast(
            mismatched,
            shanghai(),
            QDateTime::currentDateTimeUtc())),
        WeatherServiceError::InvalidResponse);
}

QTEST_APPLESS_MAIN(OpenMeteoParserTest)

#include "OpenMeteoParserTest.moc"
