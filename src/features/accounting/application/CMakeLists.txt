add_library(StuKitAccountingApplication STATIC
    CreateAccount.cpp
    CreateAccount.h
    CreateCategory.cpp
    CreateCategory.h
    CreateTransaction.cpp
    CreateTransaction.h
    DeleteTransaction.cpp
    DeleteTransaction.h
    GetAccountingAnalytics.cpp
    GetAccountingAnalytics.h
    GetAccountBalances.cpp
    GetAccountBalances.h
    GetMonthlyOverview.cpp
    GetMonthlyOverview.h
    GetTransaction.cpp
    GetTransaction.h
    GetTransactionExportData.cpp
    GetTransactionExportData.h
    GetAccountingSetup.cpp
    GetAccountingSetup.h
    UpdateAccount.cpp
    UpdateAccount.h
    UpdateCategory.cpp
    UpdateCategory.h
    UpdateTransaction.cpp
    UpdateTransaction.h
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
