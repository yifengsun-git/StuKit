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
