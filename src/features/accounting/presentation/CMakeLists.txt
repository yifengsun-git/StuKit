add_library(StuKitAccountingPresentation STATIC
    AccountListModel.cpp
    AccountListModel.h
    AccountingOverviewViewModel.cpp
    AccountingOverviewViewModel.h
    AccountingSetupViewModel.cpp
    AccountingSetupViewModel.h
    CategoryListModel.cpp
    CategoryListModel.h
    CreateTransactionViewModel.cpp
    CreateTransactionViewModel.h
    MoneyFormatter.cpp
    MoneyFormatter.h
    MoneyInputParser.cpp
    MoneyInputParser.h
    TransactionListModel.cpp
    TransactionListModel.h
)

target_include_directories(StuKitAccountingPresentation
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(StuKitAccountingPresentation
    PUBLIC
        Qt6::Core
        StuKitAccountingApplication
)

target_compile_features(StuKitAccountingPresentation
    PUBLIC
        cxx_std_20
)
