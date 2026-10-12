#include "ScheduleManagementViewModel.h"

#include <QDate>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace stukit::schedule::presentation {

ScheduleManagementViewModel::ScheduleManagementViewModel(
    GetScheduleSetup &getScheduleSetup,
    SaveAcademicTerm &saveAcademicTerm,
    SaveCourse &saveCourse,
    SaveCourseMeeting &saveCourseMeeting,
    QObject *parent)
    : QObject(parent)
    , m_getScheduleSetup(getScheduleSetup)
    , m_saveAcademicTerm(saveAcademicTerm)
    , m_saveCourse(saveCourse)
    , m_saveCourseMeeting(saveCourseMeeting)
{
}

QAbstractItemModel *ScheduleManagementViewModel::terms() noexcept
{
    return &m_termsModel;
}

QAbstractItemModel *ScheduleManagementViewModel::courses() noexcept
{
    return &m_coursesModel;
}

QAbstractItemModel *ScheduleManagementViewModel::meetings() noexcept
{
    return &m_meetingsModel;
}

QString ScheduleManagementViewModel::selectedTermId() const
{
    return m_selectedTermId.toString(QUuid::WithoutBraces);
}

QString ScheduleManagementViewModel::selectedCourseId() const
{
    return m_selectedCourseId.toString(QUuid::WithoutBraces);
}

bool ScheduleManagementViewModel::selectedTermArchived() const noexcept
{
    const AcademicTerm *term = findTerm(m_selectedTermId);
    return term != nullptr && term->isArchived();
}

int ScheduleManagementViewModel::selectedTermTotalWeeks() const noexcept
{
    const AcademicTerm *term = findTerm(m_selectedTermId);
    return term != nullptr ? term->totalWeeks() : 0;
}

bool ScheduleManagementViewModel::hasTerms() const noexcept
{
    return !m_terms.empty();
}

bool ScheduleManagementViewModel::hasCourses() const noexcept
{
    return m_coursesModel.rowCount() > 0;
}

bool ScheduleManagementViewModel::hasMeetings() const noexcept
{
    return m_meetingsModel.rowCount() > 0;
}

QVariantList ScheduleManagementViewModel::conflicts() const
{
    QVariantList result;
    result.reserve(static_cast<qsizetype>(m_conflicts.size()));
    for (const ScheduleConflict &conflict : m_conflicts) {
        result.push_back(QVariantMap{
            {QStringLiteral("courseName"), conflict.course.name()},
            {QStringLiteral("meetingId"),
             conflict.meeting.id().toString(QUuid::WithoutBraces)},
            {QStringLiteral("weekday"),
             static_cast<int>(conflict.meeting.weekday())},
            {QStringLiteral("startPeriod"),
             conflict.meeting.startPeriod()},
            {QStringLiteral("endPeriod"), conflict.meeting.endPeriod()},
            {QStringLiteral("firstConflictingWeek"),
             conflict.firstConflictingWeek},
        });
    }
    return result;
}

bool ScheduleManagementViewModel::hasConflicts() const noexcept
{
    return !m_conflicts.empty();
}

bool ScheduleManagementViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &ScheduleManagementViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

void ScheduleManagementViewModel::load()
{
    const auto result = m_getScheduleSetup.execute();
    if (std::holds_alternative<GetScheduleSetupError>(result)) {
        m_terms.clear();
        m_courses.clear();
        m_meetings.clear();
        m_selectedTermId = {};
        m_selectedCourseId = {};
        m_termsModel.clear();
        m_coursesModel.clear();
        m_meetingsModel.clear();
        setError(QStringLiteral("无法读取课表管理数据"));
        clearConflicts();
        emit selectionChanged();
        emit setupChanged();
        return;
    }

    ScheduleSetup setup = std::get<ScheduleSetup>(result);
    m_terms = std::move(setup.terms);
    m_courses = std::move(setup.courses);
    m_meetings = std::move(setup.meetings);

    if (findTerm(m_selectedTermId) == nullptr) {
        const auto active = std::ranges::find_if(
            m_terms,
            [](const AcademicTerm &term) {
                return !term.isArchived();
            });
        m_selectedTermId = active != m_terms.cend()
            ? active->id()
            : (m_terms.empty() ? QUuid{} : m_terms.front().id());
    }
    m_termsModel.replaceItems(m_terms);
    rebuildFilteredModels();
    clearError();
    clearConflicts();
    emit selectionChanged();
    emit setupChanged();
}

bool ScheduleManagementViewModel::selectTerm(const QString &termId)
{
    const QUuid id(termId.trimmed());
    if (findTerm(id) == nullptr) {
        setError(QStringLiteral("所选学期不存在"));
        return false;
    }
    if (m_selectedTermId == id) {
        clearError();
        return true;
    }

    m_selectedTermId = id;
    m_selectedCourseId = {};
    rebuildFilteredModels();
    clearError();
    clearConflicts();
    emit selectionChanged();
    emit setupChanged();
    return true;
}

bool ScheduleManagementViewModel::selectCourse(const QString &courseId)
{
    const QUuid id(courseId.trimmed());
    const Course *course = findCourse(id);
    if (course == nullptr || course->termId() != m_selectedTermId) {
        setError(QStringLiteral("所选课程不存在"));
        return false;
    }
    if (m_selectedCourseId == id) {
        clearError();
        return true;
    }

    m_selectedCourseId = id;
    rebuildMeetingModel();
    clearError();
    clearConflicts();
    emit selectionChanged();
    emit setupChanged();
    return true;
}

bool ScheduleManagementViewModel::saveTerm(
    const QString &termId,
    const QString &name,
    const QString &startDateText,
    int totalWeeks,
    bool archived)
{
    clearConflicts();
    const QString normalizedId = termId.trimmed();
    const QUuid id(normalizedId);
    if (!normalizedId.isEmpty() && id.isNull()) {
        setError(QStringLiteral("学期标识无效"));
        return false;
    }
    const auto result = m_saveAcademicTerm.execute(SaveAcademicTermRequest{
        .id = id,
        .name = name,
        .startDate = QDate::fromString(startDateText.trimmed(), Qt::ISODate),
        .totalWeeks = totalWeeks,
        .archived = archived,
    });
    if (std::holds_alternative<SaveAcademicTermError>(result)) {
        switch (std::get<SaveAcademicTermError>(result)) {
        case SaveAcademicTermError::InvalidInput:
            setError(QStringLiteral("请检查学期名称、开始日期和总周数"));
            break;
        case SaveAcademicTermError::NotFound:
            setError(QStringLiteral("要修改的学期不存在"));
            break;
        case SaveAcademicTermError::HasOutOfRangeMeetings:
            setError(QStringLiteral("现有课程安排超出了新的学期周数"));
            break;
        case SaveAcademicTermError::StorageFailure:
            setError(QStringLiteral("无法保存学期"));
            break;
        }
        return false;
    }

    m_selectedTermId = std::get<AcademicTerm>(result).id();
    load();
    emit dataChanged();
    return true;
}

bool ScheduleManagementViewModel::setTermArchived(
    const QString &termId, bool archived)
{
    const QUuid id(termId.trimmed());
    const AcademicTerm *term = findTerm(id);
    if (term == nullptr) {
        setError(QStringLiteral("要修改的学期不存在"));
        return false;
    }
    return saveTerm(
        termId,
        term->name(),
        term->startDate().toString(Qt::ISODate),
        term->totalWeeks(),
        archived);
}

bool ScheduleManagementViewModel::saveCourseDetails(
    const QString &courseId,
    const QString &termId,
    const QString &name,
    const QString &teacher,
    const QString &location,
    const QString &color)
{
    clearConflicts();
    const QString normalizedCourseId = courseId.trimmed();
    const QUuid id(normalizedCourseId);
    if (!normalizedCourseId.isEmpty() && id.isNull()) {
        setError(QStringLiteral("课程标识无效"));
        return false;
    }
    const auto result = m_saveCourse.execute(SaveCourseRequest{
        .id = id,
        .termId = QUuid(termId.trimmed()),
        .name = name,
        .teacher = teacher,
        .location = location,
        .color = color,
    });
    if (std::holds_alternative<SaveCourseError>(result)) {
        switch (std::get<SaveCourseError>(result)) {
        case SaveCourseError::InvalidInput:
            setError(QStringLiteral("请检查课程名称、教师、教室和颜色"));
            break;
        case SaveCourseError::CourseNotFound:
            setError(QStringLiteral("要修改的课程不存在"));
            break;
        case SaveCourseError::TermNotFound:
            setError(QStringLiteral("所属学期不存在"));
            break;
        case SaveCourseError::TermArchived:
            setError(QStringLiteral("已归档学期不能新增或修改课程"));
            break;
        case SaveCourseError::TermCannotChange:
            setError(QStringLiteral("不能把已有课程移动到另一学期"));
            break;
        case SaveCourseError::StorageFailure:
            setError(QStringLiteral("无法保存课程"));
            break;
        }
        return false;
    }

    const Course &saved = std::get<Course>(result);
    m_selectedTermId = saved.termId();
    m_selectedCourseId = saved.id();
    load();
    emit dataChanged();
    return true;
}

bool ScheduleManagementViewModel::saveMeetingDetails(
    const QString &meetingId,
    const QString &courseId,
    int weekday,
    int startPeriod,
    int endPeriod,
    int startWeek,
    int endWeek,
    int weekPattern)
{
    clearConflicts();
    const QString normalizedMeetingId = meetingId.trimmed();
    const QUuid id(normalizedMeetingId);
    if (!normalizedMeetingId.isEmpty() && id.isNull()) {
        setError(QStringLiteral("上课安排标识无效"));
        return false;
    }
    const auto result = m_saveCourseMeeting.execute(
        SaveCourseMeetingRequest{
            .id = id,
            .courseId = QUuid(courseId.trimmed()),
            .weekday = static_cast<Weekday>(weekday),
            .startPeriod = startPeriod,
            .endPeriod = endPeriod,
            .startWeek = startWeek,
            .endWeek = endWeek,
            .weekPattern = static_cast<WeekPattern>(weekPattern),
        });
    if (std::holds_alternative<CourseMeetingConflicts>(result)) {
        setConflicts(std::get<CourseMeetingConflicts>(result).items);
        setError(QStringLiteral("该安排与现有课程冲突"));
        return false;
    }
    if (std::holds_alternative<SaveCourseMeetingError>(result)) {
        switch (std::get<SaveCourseMeetingError>(result)) {
        case SaveCourseMeetingError::InvalidInput:
            setError(QStringLiteral("请检查星期、节次和周次"));
            break;
        case SaveCourseMeetingError::MeetingNotFound:
            setError(QStringLiteral("要修改的上课安排不存在"));
            break;
        case SaveCourseMeetingError::CourseNotFound:
            setError(QStringLiteral("所属课程不存在"));
            break;
        case SaveCourseMeetingError::TermNotFound:
            setError(QStringLiteral("所属学期不存在"));
            break;
        case SaveCourseMeetingError::TermArchived:
            setError(QStringLiteral("已归档学期不能新增或修改上课安排"));
            break;
        case SaveCourseMeetingError::MeetingExceedsTerm:
            setError(QStringLiteral("上课安排超出了学期总周数"));
            break;
        case SaveCourseMeetingError::StorageFailure:
            setError(QStringLiteral("无法保存上课安排"));
            break;
        }
        return false;
    }

    m_selectedCourseId = std::get<CourseMeeting>(result).courseId();
    const Course *course = findCourse(m_selectedCourseId);
    if (course != nullptr) {
        m_selectedTermId = course->termId();
    }
    load();
    emit dataChanged();
    return true;
}

void ScheduleManagementViewModel::clearMessages()
{
    clearError();
    clearConflicts();
}

const AcademicTerm *ScheduleManagementViewModel::findTerm(
    const QUuid &id) const
{
    const auto found = std::ranges::find_if(
        m_terms,
        [&id](const AcademicTerm &term) {
            return !id.isNull() && term.id() == id;
        });
    return found == m_terms.cend() ? nullptr : &*found;
}

const Course *ScheduleManagementViewModel::findCourse(
    const QUuid &id) const
{
    const auto found = std::ranges::find_if(
        m_courses,
        [&id](const Course &course) {
            return !id.isNull() && course.id() == id;
        });
    return found == m_courses.cend() ? nullptr : &*found;
}

void ScheduleManagementViewModel::rebuildFilteredModels()
{
    std::vector<Course> courses;
    for (const Course &course : m_courses) {
        if (course.termId() == m_selectedTermId) {
            courses.push_back(course);
        }
    }
    const bool selectedCourseStillVisible = std::ranges::any_of(
        courses,
        [this](const Course &course) {
            return course.id() == m_selectedCourseId;
        });
    if (!selectedCourseStillVisible) {
        m_selectedCourseId = courses.empty()
            ? QUuid{}
            : courses.front().id();
    }
    m_coursesModel.replaceItems(std::move(courses));
    rebuildMeetingModel();
}

void ScheduleManagementViewModel::rebuildMeetingModel()
{
    std::vector<CourseMeeting> meetings;
    for (const CourseMeeting &meeting : m_meetings) {
        if (meeting.courseId() == m_selectedCourseId) {
            meetings.push_back(meeting);
        }
    }
    m_meetingsModel.replaceItems(std::move(meetings));
}

void ScheduleManagementViewModel::setConflicts(
    std::vector<ScheduleConflict> conflicts)
{
    m_conflicts = std::move(conflicts);
    emit conflictsChanged();
}

void ScheduleManagementViewModel::clearConflicts()
{
    if (m_conflicts.empty()) {
        return;
    }
    m_conflicts.clear();
    emit conflictsChanged();
}

void ScheduleManagementViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void ScheduleManagementViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::schedule::presentation
