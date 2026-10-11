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

add_executable(StuKitCreateAccountTest
    CreateAccountTest.cpp
    support/InMemoryAccountRepository.h
)

target_link_libraries(StuKitCreateAccountTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.create_account
    COMMAND StuKitCreateAccountTest
)

add_executable(StuKitCategoryRepositoryTest
    CategoryRepositoryTest.cpp
    support/InMemoryCategoryRepository.h
)

target_link_libraries(StuKitCategoryRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.category_repository
    COMMAND StuKitCategoryRepositoryTest
)

add_executable(StuKitCreateCategoryTest
    CreateCategoryTest.cpp
    support/InMemoryCategoryRepository.h
)

target_link_libraries(StuKitCreateCategoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.create_category
    COMMAND StuKitCreateCategoryTest
)

add_executable(StuKitTransactionRepositoryTest
    TransactionRepositoryTest.cpp
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitTransactionRepositoryTest
    PRIVATE
        Qt6::Test
        StuKitAccountingDomain
)

add_test(
    NAME accounting.transaction_repository
    COMMAND StuKitTransactionRepositoryTest
)

add_executable(StuKitCreateTransactionTest
    CreateTransactionTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitCreateTransactionTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.create_transaction
    COMMAND StuKitCreateTransactionTest
)

add_executable(StuKitManageTransactionTest
    ManageTransactionTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitManageTransactionTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.manage_transaction
    COMMAND StuKitManageTransactionTest
)

add_executable(StuKitManageSetupTest
    ManageSetupTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
)

target_link_libraries(StuKitManageSetupTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.manage_setup
    COMMAND StuKitManageSetupTest
)

add_executable(StuKitGetMonthlyOverviewTest
    GetMonthlyOverviewTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitGetMonthlyOverviewTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.get_monthly_overview
    COMMAND StuKitGetMonthlyOverviewTest
)

add_executable(StuKitGetAccountBalancesTest
    GetAccountBalancesTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitGetAccountBalancesTest
    PRIVATE
        Qt6::Test
        StuKitAccountingApplication
)

add_test(
    NAME accounting.get_account_balances
    COMMAND StuKitGetAccountBalancesTest
)

add_executable(StuKitAccountingPresentationTest
    AccountingPresentationTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitAccountingPresentationTest
    PRIVATE
        Qt6::Test
        StuKitAccountingPresentation
)

add_test(
    NAME accounting.presentation
    COMMAND StuKitAccountingPresentationTest
)

add_executable(StuKitAccountBalancesPresentationTest
    AccountBalancesPresentationTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitAccountBalancesPresentationTest
    PRIVATE
        Qt6::Test
        StuKitAccountingPresentation
)

add_test(
    NAME accounting.balances_presentation
    COMMAND StuKitAccountBalancesPresentationTest
)

add_executable(StuKitTransactionFilterProxyModelTest
    TransactionFilterProxyModelTest.cpp
)

target_link_libraries(StuKitTransactionFilterProxyModelTest
    PRIVATE
        Qt6::Test
        StuKitAccountingPresentation
)

add_test(
    NAME accounting.transaction_filter_proxy
    COMMAND StuKitTransactionFilterProxyModelTest
)

add_executable(StuKitAccountingSetupTest
    AccountingSetupTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
)

target_link_libraries(StuKitAccountingSetupTest
    PRIVATE
        Qt6::Test
        StuKitAccountingPresentation
)

add_test(
    NAME accounting.setup
    COMMAND StuKitAccountingSetupTest
)

add_executable(StuKitCreateTransactionPresentationTest
    CreateTransactionPresentationTest.cpp
    support/InMemoryAccountRepository.h
    support/InMemoryCategoryRepository.h
    support/InMemoryTransactionRepository.h
)

target_link_libraries(StuKitCreateTransactionPresentationTest
    PRIVATE
        Qt6::Test
        StuKitAccountingPresentation
)

add_test(
    NAME accounting.create_transaction_presentation
    COMMAND StuKitCreateTransactionPresentationTest
)
