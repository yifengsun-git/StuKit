#include "export/CsvTransactionWriter.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace stukit::accounting;

namespace {

Transaction makeTransaction(TransactionType type, const QString &note)
{
    const auto transaction = Transaction::create({
        .id = QUuid(QStringLiteral("{01234567-89ab-cdef-0123-456789abcdef}")),
        .type = type,
        .amount = Money::fromCents(12345),
        .accountId = QUuid::createUuid(),
        .categoryId = QUuid::createUuid(),
        .occurredAt = QDateTime::fromString(
            QStringLiteral("2026-10-10T04:34:56.789Z"),
            Qt::ISODateWithMs),
        .note = note,
    });
    Q_ASSERT(transaction.has_value());
    return *transaction;
}

QByteArray readAll(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

} // namespace

class CsvTransactionWriterTest final : public QObject
{
    Q_OBJECT

private slots:
    void writesUtf8HeaderAndEscapedTransaction();
    void writesHeaderForEmptyData();
    void protectsExistingDestinationUnlessReplacementIsRequested();
    void rejectsInvalidInput();
};

void CsvTransactionWriterTest::writesUtf8HeaderAndEscapedTransaction()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-csv-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("transactions.csv"));
    TransactionExportData data{
        .transactions = {{
            .transaction = makeTransaction(
                TransactionType::Expense,
                QStringLiteral("午饭,\"套餐\"\n含饮料")),
            .accountName = QStringLiteral("现金,\"钱包\""),
            .categoryName = QStringLiteral("餐饮"),
        }},
    };
    const QTimeZone timeZone = QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60);

    const CsvExportResult result = CsvTransactionWriter::write(
        path, data, timeZone);

    QVERIFY2(result.succeeded(), qPrintable(result.detail));
    QCOMPARE(result.rowCount, qsizetype{1});
    const QByteArray contents = readAll(path);
    QVERIFY(contents.startsWith(QByteArray("\xEF\xBB\xBF", 3)));
    const QString text = QString::fromUtf8(contents.mid(3));
    QVERIFY(text.startsWith(QStringLiteral(
        "\"账单ID\",\"类型\",\"金额\",\"账户\",\"分类\","
        "\"UTC时间\",\"记账时区时间\",\"时区\",\"备注\"\r\n")));
    QVERIFY(text.contains(QStringLiteral(
        "\"01234567-89ab-cdef-0123-456789abcdef\",\"支出\",\"123.45\","
        "\"现金,\"\"钱包\"\"\",\"餐饮\","
        "\"2026-10-10T04:34:56.789Z\","
        "\"2026-10-10 12:34:56\",\"UTC+08:00\","
        "\"午饭,\"\"套餐\"\"\n含饮料\"\r\n")));
}

void CsvTransactionWriterTest::writesHeaderForEmptyData()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-empty-csv-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("empty.csv"));

    const auto result = CsvTransactionWriter::write(
        path,
        {},
        QTimeZone::utc());

    QVERIFY(result.succeeded());
    QCOMPARE(result.rowCount, qsizetype{0});
    QCOMPARE(readAll(path).count("\r\n"), 1);
}

void CsvTransactionWriterTest::protectsExistingDestinationUnlessReplacementIsRequested()
{
    QTemporaryDir directory(QDir::current().filePath(
        QStringLiteral("stukit-replace-csv-test-XXXXXX")));
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("existing.csv"));
    QFile existing(path);
    QVERIFY(existing.open(QIODevice::WriteOnly));
    QCOMPARE(existing.write("original"), qint64{8});
    existing.close();

    const auto protectedResult = CsvTransactionWriter::write(
        path, {}, QTimeZone::utc());
    QCOMPARE(protectedResult.error, CsvExportError::TargetExists);
    QCOMPARE(readAll(path), QByteArray("original"));

    const auto replacedResult = CsvTransactionWriter::write(
        path, {}, QTimeZone::utc(), true);
    QVERIFY(replacedResult.succeeded());
    QVERIFY(readAll(path).startsWith(QByteArray("\xEF\xBB\xBF", 3)));
}

void CsvTransactionWriterTest::rejectsInvalidInput()
{
    QCOMPARE(CsvTransactionWriter::write({}, {}, QTimeZone::utc()).error,
             CsvExportError::InvalidPath);
    QCOMPARE(CsvTransactionWriter::write(
                 QStringLiteral("unused.csv"), {}, QTimeZone()).error,
             CsvExportError::InvalidTimeZone);
}

QTEST_APPLESS_MAIN(CsvTransactionWriterTest)

#include "CsvTransactionWriterTest.moc"
