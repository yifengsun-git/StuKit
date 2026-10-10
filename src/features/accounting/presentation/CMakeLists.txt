add_library(StuKitAccountingPresentation STATIC
    AccountingOverviewViewModel.cpp
    AccountingOverviewViewModel.h
    MoneyFormatter.cpp
    MoneyFormatter.h
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
