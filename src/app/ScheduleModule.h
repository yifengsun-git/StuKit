#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>

#include <memory>

namespace stukit::schedule {
class GetAcademicTerms;
class DeleteScheduleItem;
class CsvScheduleImportService;
class GetScheduleSetup;
class GetWeeklySchedule;
class SaveAcademicTerm;
class SaveCourse;
class SaveCourseMeeting;
class SqlitePeriodDefinitionService;
class SqliteAcademicTermRepository;
class SqliteCourseMeetingRepository;
class SqliteCourseRepository;

namespace presentation {
class ScheduleManagementViewModel;
class ScheduleViewModel;
}
} // namespace stukit::schedule

namespace stukit::app {

class ScheduleModule final
{
public:
    explicit ScheduleModule(QSqlDatabase database);
    ~ScheduleModule();

    ScheduleModule(const ScheduleModule &) = delete;
    ScheduleModule &operator=(const ScheduleModule &) = delete;
    ScheduleModule(ScheduleModule &&) = delete;
    ScheduleModule &operator=(ScheduleModule &&) = delete;

    [[nodiscard]] bool initialize(
        const QDate &today = QDate::currentDate());

    [[nodiscard]] schedule::presentation::ScheduleViewModel *
    overviewViewModel() const noexcept;
    [[nodiscard]] schedule::presentation::ScheduleManagementViewModel *
    managementViewModel() const noexcept;
    [[nodiscard]] const QDate &today() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

private:
    void reset();

    QSqlDatabase m_database;
    QDate m_today;
    QString m_lastError;
    std::unique_ptr<schedule::SqliteAcademicTermRepository>
        m_termRepository;
    std::unique_ptr<schedule::SqliteCourseRepository> m_courseRepository;
    std::unique_ptr<schedule::SqliteCourseMeetingRepository>
        m_meetingRepository;
    std::unique_ptr<schedule::GetAcademicTerms> m_getAcademicTerms;
    std::unique_ptr<schedule::GetScheduleSetup> m_getScheduleSetup;
    std::unique_ptr<schedule::GetWeeklySchedule> m_getWeeklySchedule;
    std::unique_ptr<schedule::SaveAcademicTerm> m_saveAcademicTerm;
    std::unique_ptr<schedule::SaveCourse> m_saveCourse;
    std::unique_ptr<schedule::SaveCourseMeeting> m_saveCourseMeeting;
    std::unique_ptr<schedule::DeleteScheduleItem> m_deleteScheduleItem;
    std::unique_ptr<schedule::CsvScheduleImportService>
        m_csvImportService;
    std::unique_ptr<schedule::SqlitePeriodDefinitionService>
        m_periodDefinitionService;
    std::unique_ptr<schedule::presentation::ScheduleViewModel>
        m_overviewViewModel;
    std::unique_ptr<schedule::presentation::ScheduleManagementViewModel>
        m_managementViewModel;
};

} // namespace stukit::app
