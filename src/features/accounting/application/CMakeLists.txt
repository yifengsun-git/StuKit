add_library(StuKitAccountingApplication STATIC
    CreateAccount.cpp
    CreateAccount.h
    CreateCategory.cpp
    CreateCategory.h
    CreateTransaction.cpp
    CreateTransaction.h
    GetMonthlyOverview.cpp
    GetMonthlyOverview.h
)

target_include_directories(StuKitAccountingApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingApplication
    PUBLIC
        StuKitAccountingDomain
)

target_compile_features(StuKitAccountingApplication
    PUBLIC
        cxx_std_20
)
