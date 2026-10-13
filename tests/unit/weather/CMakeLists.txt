add_executable(StuKitWeatherDomainTest
    WeatherDomainTest.cpp
)

target_link_libraries(StuKitWeatherDomainTest
    PRIVATE
        Qt6::Test
        StuKitWeatherDomain
)

add_test(
    NAME weather.domain
    COMMAND StuKitWeatherDomainTest
)

add_executable(StuKitWeatherApplicationTest
    WeatherApplicationTest.cpp
)

target_link_libraries(StuKitWeatherApplicationTest
    PRIVATE
        Qt6::Test
        StuKitWeatherApplication
)

add_test(
    NAME weather.application
    COMMAND StuKitWeatherApplicationTest
)

add_executable(StuKitWeatherPresentationTest
    WeatherPresentationTest.cpp
)

target_link_libraries(StuKitWeatherPresentationTest
    PRIVATE
        Qt6::Test
        StuKitWeatherPresentation
)

add_test(
    NAME weather.presentation
    COMMAND StuKitWeatherPresentationTest
)
