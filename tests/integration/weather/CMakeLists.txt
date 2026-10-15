add_executable(StuKitQWeatherParserTest
    QWeatherParserTest.cpp
)

target_link_libraries(StuKitQWeatherParserTest
    PRIVATE
        Qt6::Test
        Qt6::Network
        StuKitWeatherInfrastructure
)

add_test(
    NAME weather.qweather_parser
    COMMAND StuKitQWeatherParserTest
)

add_executable(StuKitWeatherCacheTest
    JsonWeatherForecastCacheTest.cpp
)

target_link_libraries(StuKitWeatherCacheTest
    PRIVATE
        Qt6::Test
        StuKitWeatherInfrastructure
)

add_test(
    NAME weather.json_cache
    COMMAND StuKitWeatherCacheTest
)
