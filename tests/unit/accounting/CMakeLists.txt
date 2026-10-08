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

add_executable(StuKitAccountTest
    AccountTest.cpp
)

target_link_libraries(StuKitAccountTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.account
    COMMAND StuKitAccountTest
)

add_executable(StuKitCategoryTest
    CategoryTest.cpp
)

target_link_libraries(StuKitCategoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.category
    COMMAND StuKitCategoryTest
)

add_executable(StuKitAccountRepositoryTest
    AccountRepositoryTest.cpp
    support/InMemoryAccountRepository.h
)

target_link_libraries(StuKitAccountRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.account_repository
    COMMAND StuKitAccountRepositoryTest
)
