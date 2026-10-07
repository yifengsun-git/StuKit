add_executable(StuKitMoneyTest
    MoneyTest.cpp
)

target_link_libraries(StuKitMoneyTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.money
    COMMAND StuKitMoneyTest
)
