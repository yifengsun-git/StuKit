add_executable(StuKitOpenMeteoParserTest
    OpenMeteoParserTest.cpp
)

target_link_libraries(StuKitOpenMeteoParserTest
    PRIVATE
        Qt6::Test
        Qt6::Network
        StuKitWeatherInfrastructure
)

add_test(
    NAME weather.openmeteo_parser
    COMMAND StuKitOpenMeteoParserTest
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
