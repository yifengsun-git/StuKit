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

add_executable(StuKitTransactionTest
    TransactionTest.cpp
)

target_link_libraries(StuKitTransactionTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.transaction
    COMMAND StuKitTransactionTest
)
