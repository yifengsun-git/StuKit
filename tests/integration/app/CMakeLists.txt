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

add_executable(StuKitScheduleModuleIntegrationTest
    ScheduleModuleTest.cpp
)

target_link_libraries(StuKitScheduleModuleIntegrationTest
    PRIVATE
        Qt6::Sql
        Qt6::Test
        StuKitAppCore
)

add_test(
    NAME app.schedule_module
    COMMAND StuKitScheduleModuleIntegrationTest
)

add_executable(StuKitApplicationStorageIntegrationTest
    ApplicationStorageTest.cpp
)

target_link_libraries(StuKitApplicationStorageIntegrationTest
    PRIVATE
        Qt6::Test
        StuKitAppCore
)

add_test(
    NAME app.application_storage
    COMMAND StuKitApplicationStorageIntegrationTest
)
