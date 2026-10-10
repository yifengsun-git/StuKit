#include "repositories/SqliteAccountRepository.h"
#include "CreateAccount.h"
#include "database/SqliteDatabase.h"

#include <QDir>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include <memory>
#include <variant>

using namespace stukit::accounting;

namespace {

QString uniqueConnectionName()
{
    return QStringLiteral("stukit-account-repository-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

Account makeAccount(const QUuid &id, const QString &name, qint64 openingBalanceCents = 0,
                    bool archived = false)
{
    const auto account = Account::create(AccountData{
        .id = id,
        .name = name,
        .openingBalance = Money::fromCents(openingBalanceCents),
        .archived = archived,
    });

    Q_ASSERT(account.has_value());
    return *account;
}

} // namespace

class SqliteAccountRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void savesAndFindsAccount();
    void returnsNulloptForMissingAccount();
    void updatesAccountWithSameId();
    void returnsAllAccounts();
    void worksWithCreateAccountUseCase();

private:
    std::unique_ptr<QTemporaryDir> m_directory;
    std::unique_ptr<SqliteDatabase> m_database;
    std::unique_ptr<SqliteAccountRepository> m_repository;
};

void SqliteAccountRepositoryTest::init()
{
    m_directory = std::make_unique<QTemporaryDir>(QDir::current().filePath(
        QStringLiteral("stukit-account-repository-test-XXXXXX")));
    QVERIFY(m_directory->isValid());

    m_database = std::make_unique<SqliteDatabase>(uniqueConnectionName());
    QVERIFY2(m_database->open(m_directory->filePath(QStringLiteral("stukit.sqlite"))),
             qPrintable(m_database->lastError()));
    QVERIFY2(m_database->migrate(), qPrintable(m_database->lastError()));

    m_repository = std::make_unique<SqliteAccountRepository>(m_database->connection());
}

void SqliteAccountRepositoryTest::cleanup()
{
    m_repository.reset();
    m_database.reset();
    m_directory.reset();
}

void SqliteAccountRepositoryTest::savesAndFindsAccount()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeAccount(id, QStringLiteral("现金"), -1234, true)));

    const auto stored = m_repository->findById(id);
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->id(), id);
    QCOMPARE(stored->name(), QStringLiteral("现金"));
    QCOMPARE(stored->openingBalance().cents(), qint64{-1234});
    QVERIFY(stored->isArchived());
}

void SqliteAccountRepositoryTest::returnsNulloptForMissingAccount()
{
    const auto stored = m_repository->findById(QUuid::createUuid());

    QVERIFY(!stored.has_value());
    QVERIFY(m_repository->lastError().isEmpty());
}

void SqliteAccountRepositoryTest::updatesAccountWithSameId()
{
    const auto id = QUuid::createUuid();
    QVERIFY(m_repository->save(makeAccount(id, QStringLiteral("现金"))));
    QVERIFY(m_repository->save(makeAccount(id, QStringLiteral("日常账户"), 5000)));

    const auto accounts = m_repository->findAll();
    QVERIFY2(m_repository->lastError().isEmpty(), qPrintable(m_repository->lastError()));
    QCOMPARE(accounts.size(), std::size_t{1});
    QCOMPARE(accounts.front().name(), QStringLiteral("日常账户"));
    QCOMPARE(accounts.front().openingBalance().cents(), qint64{5000});
}

void SqliteAccountRepositoryTest::returnsAllAccounts()
{
    QVERIFY(m_repository->save(makeAccount(QUuid::createUuid(), QStringLiteral("现金"))));
    QVERIFY(
        m_repository->save(makeAccount(QUuid::createUuid(), QStringLiteral("银行卡"))));

    const auto accounts = m_repository->findAll();

    QVERIFY2(m_repository->lastError().isEmpty(), qPrintable(m_repository->lastError()));
    QCOMPARE(accounts.size(), std::size_t{2});
}

void SqliteAccountRepositoryTest::worksWithCreateAccountUseCase()
{
    CreateAccount createAccount(*m_repository);

    const auto result = createAccount.execute(CreateAccountRequest{
        .name = QStringLiteral("支付宝"),
        .openingBalance = Money::fromCents(8800),
    });

    QVERIFY(std::holds_alternative<Account>(result));
    const auto &created = std::get<Account>(result);

    const auto stored = m_repository->findById(created.id());
    QVERIFY2(stored.has_value(), qPrintable(m_repository->lastError()));
    QCOMPARE(stored->name(), QStringLiteral("支付宝"));
    QCOMPARE(stored->openingBalance().cents(), qint64{8800});
}

QTEST_GUILESS_MAIN(SqliteAccountRepositoryTest)

#include "SqliteAccountRepositoryTest.moc"
