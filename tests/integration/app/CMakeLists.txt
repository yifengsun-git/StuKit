add_executable(StuKitAccountingModuleTest
    AccountingModuleTest.cpp
)

target_link_libraries(StuKitAccountingModuleTest
    PRIVATE
        Qt6::Sql
        Qt6::Test
        StuKitAppCore
)

add_test(
    NAME app.accounting_module
    COMMAND StuKitAccountingModuleTest
)
