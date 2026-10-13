#pragma once

#include "IScheduleImportService.h"

namespace stukit::schedule {

class ICourseMeetingRepository;
class ICourseRepository;
class SaveCourse;
class SaveCourseMeeting;

class CsvScheduleImportService final : public IScheduleImportService
{
public:
    CsvScheduleImportService(
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository,
        SaveCourse &saveCourse,
        SaveCourseMeeting &saveMeeting);

    [[nodiscard]] ScheduleImportResult importCsv(
        const QString &filePath,
        const QUuid &termId) override;

private:
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
    SaveCourse &m_saveCourse;
    SaveCourseMeeting &m_saveMeeting;
};

} // namespace stukit::schedule
