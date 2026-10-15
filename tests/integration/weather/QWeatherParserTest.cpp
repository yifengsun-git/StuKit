#include "QWeatherJsonParser.h"
#include "QWeatherUrlBuilder.h"

#include <QTest>
#include <QTimeZone>
#include <QUrlQuery>

using namespace stukit::weather;

class QWeatherParserTest final : public QObject
{
    Q_OBJECT

private slots:
    void buildsLocationUrl();
    void buildsForecastUrls();
    void rejectsUnsafeApiHost();
    void parsesLocations();
    void parsesEmptyLocations();
    void mapsProviderErrorCodes();
    void parsesForecastInLocationTimeZone();
    void rejectsMalformedJson();
};

namespace {

const QString apiHost = QStringLiteral("abc123.qweatherapi.com");

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
  "code": "200",
  "location": [{
    "name": "上海",
    "id": "101020100",
    "lat": "31.23171",
    "lon": "121.47264",
    "adm2": "上海",
    "adm1": "上海市",
    "country": "中国",
    "tz": "Asia/Shanghai"
  }]
})json";

const QByteArray currentJson = R"json({
  "metadata": {
    "attributions": ["https://developer.qweather.com/attribution.html"]
  },
  "condition": {"text": "少云", "code": "102"},
  "temperature": {"value": 23.5, "unit": "°C"},
  "feelsLike": {"value": 24.1, "unit": "°C"},
  "humidity": 0.68,
  "wind": {
    "direction": {"degree": 90, "compass": "e"},
    "speed": {"value": 3.5, "unit": "m/s"}
  },
  "precipitation": {
    "amount": {"value": 0.0, "unit": "mm"},
    "type": "none"
  }
})json";

const QByteArray dailyJson = R"json({
  "metadata": {
    "attributions": ["https://developer.qweather.com/attribution.html"]
  },
  "days": [{
    "forecastStartTime": "2026-10-13T00:00+08:00",
    "forecastEndTime": "2026-10-14T00:00+08:00",
    "astro": {
      "sunrise": "2026-10-13T05:55+08:00",
      "sunset": "2026-10-13T17:25+08:00"
    },
    "temperatureMax": {"value": 25.0, "unit": "°C"},
    "temperatureMin": {"value": 18.0, "unit": "°C"},
    "daytime": {
      "condition": {"text": "少云", "code": "102"},
      "precipitation": {
        "amount": {"value": 0.2, "unit": "mm"},
        "probability": 0.2
      }
    },
    "nighttime": {
      "condition": {"text": "晴", "code": "150"},
      "precipitation": {
        "amount": {"value": 0.0, "unit": "mm"},
        "probability": 0.1
      }
    }
  }, {
    "forecastStartTime": "2026-10-14T00:00+08:00",
    "forecastEndTime": "2026-10-15T00:00+08:00",
    "astro": {
      "sunrise": "2026-10-14T05:56+08:00",
      "sunset": "2026-10-14T17:24+08:00"
    },
    "temperatureMax": {"value": 23.0, "unit": "°C"},
    "temperatureMin": {"value": 17.0, "unit": "°C"},
    "daytime": {
      "condition": {"text": "小雨", "code": "305"},
      "precipitation": {
        "amount": {"value": 5.6, "unit": "mm"},
        "probability": 0.7
      }
    },
    "nighttime": {
      "condition": {"text": "中雨", "code": "306"},
      "precipitation": {
        "amount": {"value": 1.2, "unit": "mm"},
        "probability": 0.4
      }
    }
  }]
})json";

} // namespace

void QWeatherParserTest::buildsLocationUrl()
{
    const QUrl url = QWeatherUrlBuilder::locationSearch(
        apiHost, QStringLiteral(" 上海 "));
    const QUrlQuery query(url);
    QCOMPARE(url.scheme(), QStringLiteral("https"));
    QCOMPARE(url.host(), apiHost);
    QCOMPARE(url.path(), QStringLiteral("/geo/v2/city/lookup"));
    QCOMPARE(query.queryItemValue(QStringLiteral("location")),
             QStringLiteral("上海"));
    QCOMPARE(query.queryItemValue(QStringLiteral("lang")),
             QStringLiteral("zh"));
}

void QWeatherParserTest::buildsForecastUrls()
{
    const QUrl current = QWeatherUrlBuilder::currentWeather(apiHost, shanghai());
    QCOMPARE(current.path(),
             QStringLiteral("/weather/v1/current/31.23/121.47"));
    QCOMPARE(QUrlQuery(current).queryItemValue(QStringLiteral("localTime")),
             QStringLiteral("true"));

    const QUrl daily = QWeatherUrlBuilder::dailyForecast(apiHost, shanghai());
    QCOMPARE(daily.path(),
             QStringLiteral("/weather/v1/daily/31.23/121.47"));
    QCOMPARE(QUrlQuery(daily).queryItemValue(QStringLiteral("days")),
             QStringLiteral("7"));
}

void QWeatherParserTest::rejectsUnsafeApiHost()
{
    QVERIFY(!QWeatherUrlBuilder::isValidApiHost(
        QStringLiteral("http://example.com")));
    QVERIFY(!QWeatherUrlBuilder::isValidApiHost(
        QStringLiteral("https://user:secret@example.com")));
}

void QWeatherParserTest::parsesLocations()
{
    const LocationSearchResult result =
        QWeatherJsonParser::parseLocations(locationJson);
    QVERIFY(std::holds_alternative<std::vector<WeatherLocation>>(result));
    const auto &locations = std::get<std::vector<WeatherLocation>>(result);
    QCOMPARE(locations.size(), std::size_t{1});
    QCOMPARE(locations.front().name(), QStringLiteral("上海"));
    QCOMPARE(locations.front().timezoneId(), QStringLiteral("Asia/Shanghai"));
    QCOMPARE(locations.front().administrativeArea(),
             QStringLiteral("上海市 · 上海"));
}

void QWeatherParserTest::parsesEmptyLocations()
{
    const LocationSearchResult result = QWeatherJsonParser::parseLocations(
        QByteArrayLiteral("{\"code\":\"404\"}"));
    QVERIFY(std::holds_alternative<std::vector<WeatherLocation>>(result));
    QVERIFY(std::get<std::vector<WeatherLocation>>(result).empty());
}

void QWeatherParserTest::mapsProviderErrorCodes()
{
    QCOMPARE(
        std::get<WeatherServiceError>(QWeatherJsonParser::parseLocations(
            QByteArrayLiteral("{\"code\":\"401\"}"))),
        WeatherServiceError::AuthenticationFailed);
    QCOMPARE(
        std::get<WeatherServiceError>(QWeatherJsonParser::parseLocations(
            QByteArrayLiteral("{\"code\":\"429\"}"))),
        WeatherServiceError::RateLimited);
}

void QWeatherParserTest::parsesForecastInLocationTimeZone()
{
    const ForecastResult result = QWeatherJsonParser::parseForecast(
        currentJson,
        dailyJson,
        shanghai(),
        QDateTime(QDate(2026, 10, 13), QTime(2, 5), QTimeZone::UTC));
    QVERIFY(std::holds_alternative<WeatherForecast>(result));
    const WeatherForecast &forecast = std::get<WeatherForecast>(result);
    QCOMPARE(forecast.current().condition, WeatherCondition::MainlyClear);
    QCOMPARE(forecast.current().relativeHumidityPercent, 68);
    QCOMPARE(forecast.current().windSpeedKilometresPerHour, 12.6);
    QCOMPARE(forecast.current().observedAt.timeZone().id(),
             QByteArray("Asia/Shanghai"));
    QVERIFY(forecast.current().daytime);
    QCOMPARE(forecast.daily().size(), std::size_t{2});
    QCOMPARE(forecast.daily().back().condition, WeatherCondition::Rain);
    QCOMPARE(forecast.daily().back().precipitationProbabilityPercent, 70);
    QCOMPARE(forecast.daily().back().precipitationMillimetres, 6.8);
}

void QWeatherParserTest::rejectsMalformedJson()
{
    QCOMPARE(
        std::get<WeatherServiceError>(
            QWeatherJsonParser::parseLocations(QByteArrayLiteral("not-json"))),
        WeatherServiceError::InvalidResponse);
    QCOMPARE(
        std::get<WeatherServiceError>(QWeatherJsonParser::parseForecast(
            currentJson,
            QByteArrayLiteral("{}"),
            shanghai(),
            QDateTime::currentDateTimeUtc())),
        WeatherServiceError::InvalidResponse);
}

QTEST_APPLESS_MAIN(QWeatherParserTest)

#include "QWeatherParserTest.moc"
