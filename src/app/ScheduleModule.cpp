#include "ScheduleModule.h"

#include "GetAcademicTerms.h"
#include "GetScheduleSetup.h"
#include "GetWeeklySchedule.h"
#include "SaveAcademicTerm.h"
#include "SaveCourse.h"
#include "SaveCourseMeeting.h"
#include "ScheduleManagementViewModel.h"
#include "ScheduleViewModel.h"
#include "repositories/SqliteAcademicTermRepository.h"
#include "repositories/SqliteCourseMeetingRepository.h"
#include "repositories/SqliteCourseRepository.h"

#include <QObject>

#include <utility>

namespace stukit::app {

ScheduleModule::ScheduleModule(QSqlDatabase database)
    : m_database(std::move(database))
{
}

ScheduleModule::~ScheduleModule() = default;

bool ScheduleModule::initialize(const QDate &today)
{
    reset();
    m_lastError.clear();
    if (!m_database.isValid() || !m_database.isOpen()) {
        m_lastError = QStringLiteral("Schedule database is not open");
        return false;
    }
    if (!today.isValid()) {
        m_lastError = QStringLiteral("Schedule date is invalid");
        return false;
    }
    m_today = today;

    m_termRepository =
        std::make_unique<schedule::SqliteAcademicTermRepository>(m_database);
    m_courseRepository =
        std::make_unique<schedule::SqliteCourseRepository>(m_database);
    m_meetingRepository =
        std::make_unique<schedule::SqliteCourseMeetingRepository>(m_database);
    m_getAcademicTerms = std::make_unique<schedule::GetAcademicTerms>(
        *m_termRepository);
    m_getScheduleSetup = std::make_unique<schedule::GetScheduleSetup>(
        *m_termRepository, *m_courseRepository, *m_meetingRepository);
    m_getWeeklySchedule = std::make_unique<schedule::GetWeeklySchedule>(
        *m_termRepository, *m_courseRepository, *m_meetingRepository);
    m_saveAcademicTerm = std::make_unique<schedule::SaveAcademicTerm>(
        *m_termRepository, *m_courseRepository, *m_meetingRepository);
    m_saveCourse = std::make_unique<schedule::SaveCourse>(
        *m_termRepository, *m_courseRepository);
    m_saveCourseMeeting = std::make_unique<schedule::SaveCourseMeeting>(
        *m_termRepository, *m_courseRepository, *m_meetingRepository);
    m_overviewViewModel =
        std::make_unique<schedule::presentation::ScheduleViewModel>(
            *m_getAcademicTerms, *m_getWeeklySchedule, m_today);
    m_managementViewModel =
        std::make_unique<schedule::presentation::ScheduleManagementViewModel>(
            *m_getScheduleSetup,
            *m_saveAcademicTerm,
            *m_saveCourse,
            *m_saveCourseMeeting);

    QObject::connect(
        m_managementViewModel.get(),
        &schedule::presentation::ScheduleManagementViewModel::dataChanged,
        m_overviewViewModel.get(),
        &schedule::presentation::ScheduleViewModel::load);

    m_managementViewModel->load();
    m_overviewViewModel->load();
    if (m_managementViewModel->hasError()
        || m_overviewViewModel->hasError()) {
        m_lastError = QStringLiteral("Could not load schedule data");
        reset();
        return false;
    }
    return true;
}

schedule::presentation::ScheduleViewModel *
ScheduleModule::overviewViewModel() const noexcept
{
    return m_overviewViewModel.get();
}

schedule::presentation::ScheduleManagementViewModel *
ScheduleModule::managementViewModel() const noexcept
{
    return m_managementViewModel.get();
}

const QDate &ScheduleModule::today() const noexcept
{
    return m_today;
}

const QString &ScheduleModule::lastError() const noexcept
{
    return m_lastError;
}

void ScheduleModule::reset()
{
    m_managementViewModel.reset();
    m_overviewViewModel.reset();
    m_saveCourseMeeting.reset();
    m_saveCourse.reset();
    m_saveAcademicTerm.reset();
    m_getWeeklySchedule.reset();
    m_getScheduleSetup.reset();
    m_getAcademicTerms.reset();
    m_meetingRepository.reset();
    m_courseRepository.reset();
    m_termRepository.reset();
}

} // namespace stukit::app
