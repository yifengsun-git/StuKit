#include "CsvTransactionWriter.h"

#include <QFileInfo>
#include <QSaveFile>
#include <QStringList>

namespace stukit::accounting {

namespace {

QString formatMoney(Money money)
{
    const qint64 cents = money.cents();
    const quint64 magnitude = cents < 0
        ? static_cast<quint64>(-(cents + 1)) + 1
        : static_cast<quint64>(cents);
    return QStringLiteral("%1%2.%3")
        .arg(cents < 0 ? QStringLiteral("-") : QString{})
        .arg(magnitude / 100)
        .arg(magnitude % 100, 2, 10, QLatin1Char('0'));
}

QString quoteCsv(QString value)
{
    value.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QStringLiteral("\"%1\"").arg(value);
}

QByteArray row(const QStringList &fields)
{
    QStringList quotedFields;
    quotedFields.reserve(fields.size());
    for (const QString &field : fields) {
        quotedFields.push_back(quoteCsv(field));
    }
    return (quotedFields.join(QLatin1Char(',')) + QStringLiteral("\r\n"))
        .toUtf8();
}

} // namespace

CsvExportResult CsvTransactionWriter::write(
    const QString &destinationPath,
    const TransactionExportData &data,
    const QTimeZone &timeZone,
    bool replaceExisting)
{
    const QString path = destinationPath.trimmed();
    if (path.isEmpty()) {
        return {CsvExportError::InvalidPath,
                QStringLiteral("CSV destination path is empty")};
    }
    if (!timeZone.isValid()) {
        return {CsvExportError::InvalidTimeZone,
                QStringLiteral("Accounting time zone is invalid")};
    }
    if (QFileInfo::exists(path) && !replaceExisting) {
        return {CsvExportError::TargetExists,
                QStringLiteral("CSV destination already exists")};
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return {CsvExportError::IoFailure, file.errorString()};
    }

    const QByteArray bom("\xEF\xBB\xBF", 3);
    if (file.write(bom) != bom.size()) {
        file.cancelWriting();
        return {CsvExportError::IoFailure, file.errorString()};
    }

    const QByteArray header = row({
        QStringLiteral("账单ID"),
        QStringLiteral("类型"),
        QStringLiteral("金额"),
        QStringLiteral("账户"),
        QStringLiteral("分类"),
        QStringLiteral("UTC时间"),
        QStringLiteral("记账时区时间"),
        QStringLiteral("时区"),
        QStringLiteral("备注"),
    });
    if (file.write(header) != header.size()) {
        file.cancelWriting();
        return {CsvExportError::IoFailure, file.errorString()};
    }

    for (const TransactionExportItem &item : data.transactions) {
        const Transaction &transaction = item.transaction;
        const QByteArray transactionRow = row({
            transaction.id().toString(QUuid::WithoutBraces),
            transaction.type() == TransactionType::Income
                ? QStringLiteral("收入")
                : QStringLiteral("支出"),
            formatMoney(transaction.amount()),
            item.accountName,
            item.categoryName,
            transaction.occurredAt().toUTC().toString(Qt::ISODateWithMs),
            transaction.occurredAt().toTimeZone(timeZone).toString(
                QStringLiteral("yyyy-MM-dd HH:mm:ss")),
            QString::fromUtf8(timeZone.id()),
            transaction.note(),
        });
        if (file.write(transactionRow) != transactionRow.size()) {
            file.cancelWriting();
            return {CsvExportError::IoFailure, file.errorString()};
        }
    }

    if (!file.commit()) {
        return {CsvExportError::IoFailure, file.errorString()};
    }
    return {CsvExportError::None,
            {},
            static_cast<qsizetype>(data.transactions.size())};
}

} // namespace stukit::accounting
