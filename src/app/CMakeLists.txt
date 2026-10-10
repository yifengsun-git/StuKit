add_library(StuKitAppCore STATIC
    AccountingModule.cpp
    AccountingModule.h
    UserSettings.cpp
    UserSettings.h
)

target_include_directories(StuKitAppCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAppCore
    PUBLIC
        Qt6::Core
        StuKitAccountingInfrastructure
        StuKitAccountingPresentation
)

target_compile_features(StuKitAppCore
    PUBLIC
        cxx_std_20
)
