add_library(StuKitUpdateDomain INTERFACE)

target_include_directories(StuKitUpdateDomain
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitUpdateDomain
    INTERFACE
        Qt6::Core
)

target_compile_features(StuKitUpdateDomain INTERFACE cxx_std_20)
