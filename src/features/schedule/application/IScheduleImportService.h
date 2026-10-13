#pragma once

#include <QString>
#include <QUuid>

namespace stukit::schedule {

enum class ScheduleImportError
{
    None,
    InvalidPath,
    FileOpenFailed,
    InvalidHeader,
    InvalidRow,
    TermNotFound,
    TermArchived,
    Conflict,
    StorageFailure
};

struct ScheduleImportResult
{
    ScheduleImportError error{ScheduleImportError::None};
    int rowNumber{0};
    int importedCourses{0};
    int importedMeetings{0};
    QString detail;

    [[nodiscard]] bool succeeded() const noexcept
    {
        return error == ScheduleImportError::None;
    }
};

class IScheduleImportService
{
public:
    virtual ~IScheduleImportService() = default;

    [[nodiscard]] virtual ScheduleImportResult importCsv(
        const QString &filePath,
        const QUuid &termId) = 0;
};

} // namespace stukit::schedule
