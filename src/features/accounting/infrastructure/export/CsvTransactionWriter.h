#pragma once

#include "GetTransactionExportData.h"

#include <QString>
#include <QTimeZone>

namespace stukit::accounting {

enum class CsvExportError
{
    None,
    InvalidPath,
    TargetExists,
    InvalidTimeZone,
    IoFailure
};

struct CsvExportResult
{
    CsvExportError error{CsvExportError::None};
    QString detail;
    qsizetype rowCount{0};

    [[nodiscard]] bool succeeded() const noexcept
    {
        return error == CsvExportError::None;
    }
};

class CsvTransactionWriter final
{
public:
    [[nodiscard]] static CsvExportResult write(
        const QString &destinationPath,
        const TransactionExportData &data,
        const QTimeZone &timeZone,
        bool replaceExisting = false);
};

} // namespace stukit::accounting
