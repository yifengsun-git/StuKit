add_library(StuKitWeatherPresentation STATIC
    WeatherViewModel.cpp
    WeatherViewModel.h
)

target_include_directories(StuKitWeatherPresentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitWeatherPresentation
    PUBLIC
        Qt6::Core
        StuKitWeatherApplication
)

target_compile_features(StuKitWeatherPresentation PUBLIC cxx_std_20)
