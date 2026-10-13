add_library(StuKitWeatherDomain STATIC
    WeatherCondition.cpp
    WeatherCondition.h
    WeatherForecast.cpp
    WeatherForecast.h
    WeatherLocation.cpp
    WeatherLocation.h
)

target_include_directories(StuKitWeatherDomain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitWeatherDomain
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitWeatherDomain
    PUBLIC
        cxx_std_20
)
