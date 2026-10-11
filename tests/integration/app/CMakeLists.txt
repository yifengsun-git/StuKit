add_executable(StuKitAccountingModuleIntegrationTest
    AccountingModuleTest.cpp
)

target_link_libraries(StuKitAccountingModuleIntegrationTest
    PRIVATE
        Qt6::Sql
        Qt6::Test
        StuKitAppCore
)

add_test(
    NAME app.accounting_module
    COMMAND StuKitAccountingModuleIntegrationTest
)
