add_library(StuKitWeatherInfrastructure STATIC
    cache/JsonWeatherForecastCache.cpp
    cache/JsonWeatherForecastCache.h
    qweather/QWeatherJsonParser.cpp
    qweather/QWeatherJsonParser.h
    qweather/QWeatherUrlBuilder.cpp
    qweather/QWeatherUrlBuilder.h
    qweather/QWeatherWeatherService.cpp
    qweather/QWeatherWeatherService.h
)

target_include_directories(StuKitWeatherInfrastructure
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/cache
        ${CMAKE_CURRENT_SOURCE_DIR}/qweather
)

target_link_libraries(StuKitWeatherInfrastructure
    PUBLIC
        Qt6::Core
        Qt6::Network
        StuKitWeatherApplication
)

target_compile_features(StuKitWeatherInfrastructure
    PUBLIC
        cxx_std_20
)
