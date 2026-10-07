add_library(StuKitAccountingDomain STATIC
    Money.cpp
    Money.h
)

target_include_directories(StuKitAccountingDomain
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingDomain
    PUBLIC
        Qt6::Core
)

target_compile_features(StuKitAccountingDomain
    PUBLIC
        cxx_std_20
)
