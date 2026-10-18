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
    DeleteScheduleItem &deleteScheduleItem,
    IScheduleImportService *importService,
    IPeriodDefinitionService *periodDefinitionService,
    QObject *parent)
    : QObject(parent)
    , m_getScheduleSetup(getScheduleSetup)
    , m_saveAcademicTerm(saveAcademicTerm)
    , m_saveCourse(saveCourse)
    , m_saveCourseMeeting(saveCourseMeeting)
    , m_deleteScheduleItem(deleteScheduleItem)
    , m_importService(importService)
    , m_periodDefinitionService(periodDefinitionService)
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

QAbstractItemModel *ScheduleManagementViewModel::termSchedule() noexcept
{
    return &m_termScheduleModel;
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

const QString &ScheduleManagementViewModel::importMessage() const noexcept
{
    return m_importMessage;
}

QVariantList ScheduleManagementViewModel::periodDefinitions() const
{
    QVariantList definitions;
    definitions.reserve(static_cast<qsizetype>(m_periodDefinitions.size()));
    for (const PeriodDefinition &definition : m_periodDefinitions) {
        definitions.push_back(QVariantMap{
            {QStringLiteral("periodNumber"), definition.periodNumber},
            {QStringLiteral("name"), definition.name},
            {QStringLiteral("startTime"), definition.startTime},
            {QStringLiteral("endTime"), definition.endTime},
            {QStringLiteral("displayText"),
             QStringLiteral("%1\n%2–%3")
                 .arg(definition.name,
                      definition.startTime,
                      definition.endTime)},
        });
    }
    return definitions;
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
        m_termScheduleModel.clear();
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
    if (m_periodDefinitionService != nullptr) {
        const LoadPeriodDefinitionsResult periodResult =
            m_periodDefinitionService->load();
        if (std::holds_alternative<PeriodDefinitionError>(periodResult)) {
            m_periodDefinitions.clear();
            emit periodDefinitionsChanged();
            setError(QStringLiteral("无法读取节次时间设置"));
            emit selectionChanged();
            emit setupChanged();
            return;
        }
        m_periodDefinitions =
            std::get<std::vector<PeriodDefinition>>(periodResult);
        emit periodDefinitionsChanged();
    }
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
    return saveCourseDetails(
        courseId,
        termId,
        name,
        teacher,
        location,
        color,
        {},
        {});
}

bool ScheduleManagementViewModel::saveCourseDetails(
    const QString &courseId,
    const QString &termId,
    const QString &name,
    const QString &teacher,
    const QString &location,
    const QString &color,
    const QString &assessmentStandard,
    const QString &teachingPlan)
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
        .assessmentStandard = assessmentStandard,
        .teachingPlan = teachingPlan,
        .color = color,
    });
    if (std::holds_alternative<SaveCourseError>(result)) {
        switch (std::get<SaveCourseError>(result)) {
        case SaveCourseError::InvalidInput:
            setError(QStringLiteral(
                "请检查课程名称、教师、教室、课程资料和颜色"));
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
    return saveMeeting(
        meetingId,
        courseId,
        weekday,
        startPeriod,
        endPeriod,
        startWeek,
        endWeek,
        static_cast<WeekPattern>(weekPattern),
        {});
}

bool ScheduleManagementViewModel::saveMeetingWeeks(
    const QString &meetingId,
    const QString &courseId,
    int weekday,
    int startPeriod,
    int endPeriod,
    const QVariantList &activeWeeks)
{
    std::vector<int> weeks;
    weeks.reserve(static_cast<std::size_t>(activeWeeks.size()));
    for (const QVariant &value : activeWeeks) {
        bool isNumber = false;
        const int week = value.toInt(&isNumber);
        if (!isNumber) {
            setError(QStringLiteral("周次格式无效"));
            return false;
        }
        weeks.push_back(week);
    }
    if (weeks.empty()) {
        setError(QStringLiteral("请至少选择一个教学周"));
        return false;
    }
    std::ranges::sort(weeks);
    const auto uniqueEnd = std::ranges::unique(weeks).begin();
    weeks.erase(uniqueEnd, weeks.end());
    if (weeks.empty()) {
        setError(QStringLiteral("请至少选择一个教学周"));
        return false;
    }
    return saveMeeting(
        meetingId,
        courseId,
        weekday,
        startPeriod,
        endPeriod,
        weeks.front(),
        weeks.back(),
        WeekPattern::EveryWeek,
        std::move(weeks));
}

bool ScheduleManagementViewModel::saveMeeting(
    const QString &meetingId,
    const QString &courseId,
    int weekday,
    int startPeriod,
    int endPeriod,
    int startWeek,
    int endWeek,
    WeekPattern weekPattern,
    std::vector<int> activeWeeks)
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
            .weekPattern = weekPattern,
            .activeWeeks = std::move(activeWeeks),
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

bool ScheduleManagementViewModel::deleteMeeting(const QString &meetingId)
{
    clearMessages();
    const auto result = m_deleteScheduleItem.deleteMeeting(
        QUuid(meetingId.trimmed()));
    if (result != DeleteScheduleItemError::None) {
        switch (result) {
        case DeleteScheduleItemError::None:
            break;
        case DeleteScheduleItemError::InvalidInput:
            setError(QStringLiteral("上课安排标识无效"));
            break;
        case DeleteScheduleItemError::ItemNotFound:
            setError(QStringLiteral("要删除的上课安排不存在"));
            break;
        case DeleteScheduleItemError::RelatedItemNotFound:
            setError(QStringLiteral("上课安排关联的数据不存在"));
            break;
        case DeleteScheduleItemError::TermArchived:
            setError(QStringLiteral("已归档学期不能删除上课安排"));
            break;
        case DeleteScheduleItemError::TermNotArchived:
            setError(QStringLiteral("学期状态不允许删除上课安排"));
            break;
        case DeleteScheduleItemError::StorageFailure:
            setError(QStringLiteral("无法删除上课安排"));
            break;
        }
        return false;
    }
    load();
    emit dataChanged();
    return true;
}

bool ScheduleManagementViewModel::deleteCourse(const QString &courseId)
{
    clearMessages();
    const auto result = m_deleteScheduleItem.deleteCourse(
        QUuid(courseId.trimmed()));
    if (result != DeleteScheduleItemError::None) {
        switch (result) {
        case DeleteScheduleItemError::None:
            break;
        case DeleteScheduleItemError::InvalidInput:
            setError(QStringLiteral("课程标识无效"));
            break;
        case DeleteScheduleItemError::ItemNotFound:
            setError(QStringLiteral("要删除的课程不存在"));
            break;
        case DeleteScheduleItemError::RelatedItemNotFound:
            setError(QStringLiteral("课程所属学期不存在"));
            break;
        case DeleteScheduleItemError::TermArchived:
            setError(QStringLiteral("已归档学期不能删除课程"));
            break;
        case DeleteScheduleItemError::TermNotArchived:
            setError(QStringLiteral("学期状态不允许删除课程"));
            break;
        case DeleteScheduleItemError::StorageFailure:
            setError(QStringLiteral("无法删除课程"));
            break;
        }
        return false;
    }
    load();
    emit dataChanged();
    return true;
}

bool ScheduleManagementViewModel::deleteTerm(const QString &termId)
{
    const DeleteScheduleItemError result =
        m_deleteScheduleItem.deleteTerm(QUuid(termId));
    if (result != DeleteScheduleItem::Success) {
        setError(result == DeleteScheduleItemError::TermNotArchived
                     ? QStringLiteral("请先归档学期，再永久删除")
                     : QStringLiteral("无法删除学期"));
        return false;
    }
    m_selectedTermId = {};
    m_selectedCourseId = {};
    load();
    emit dataChanged();
    return !hasError();
}

bool ScheduleManagementViewModel::importScheduleCsv(const QUrl &fileUrl)
{
    clearMessages();
    if (m_importService == nullptr) {
        setError(QStringLiteral("当前构建未启用课表 CSV 导入"));
        return false;
    }
    if (m_selectedTermId.isNull()) {
        setError(QStringLiteral("请先创建并选择一个学期"));
        return false;
    }
    if (selectedTermArchived()) {
        setError(QStringLiteral("已归档学期不能导入课表"));
        return false;
    }
    QString sourcePath;
    if (fileUrl.isLocalFile()) {
        sourcePath = fileUrl.toLocalFile();
    } else if (fileUrl.scheme().compare(
                   QStringLiteral("content"), Qt::CaseInsensitive) == 0) {
        sourcePath = fileUrl.toString(QUrl::FullyEncoded);
    }
    if (sourcePath.isEmpty()) {
        setError(QStringLiteral("请选择 CSV 文件"));
        return false;
    }

    const ScheduleImportResult result = m_importService->importCsv(
        sourcePath, m_selectedTermId);
    if (!result.succeeded()) {
        QString message;
        switch (result.error) {
        case ScheduleImportError::None:
            break;
        case ScheduleImportError::InvalidPath:
            message = QStringLiteral("请选择有效的 CSV 文件");
            break;
        case ScheduleImportError::FileOpenFailed:
            message = QStringLiteral("无法读取所选 CSV 文件");
            break;
        case ScheduleImportError::InvalidHeader:
            message = QStringLiteral("CSV 表头不符合课表导入模板");
            break;
        case ScheduleImportError::InvalidRow:
            message = result.detail.isEmpty()
                ? QStringLiteral("CSV 数据格式无效") : result.detail;
            break;
        case ScheduleImportError::TermNotFound:
            message = QStringLiteral("当前学期不存在");
            break;
        case ScheduleImportError::TermArchived:
            message = QStringLiteral("已归档学期不能导入课表");
            break;
        case ScheduleImportError::Conflict:
            message = result.detail.isEmpty()
                ? QStringLiteral("导入内容存在课程冲突") : result.detail;
            break;
        case ScheduleImportError::StorageFailure:
            message = QStringLiteral("导入失败，数据库未写入任何内容");
            break;
        }
        if (result.rowNumber > 0) {
            message = QStringLiteral("第 %1 行：%2")
                .arg(result.rowNumber)
                .arg(message);
        }
        setError(std::move(message));
        return false;
    }

    load();
    emit dataChanged();
    setImportMessage(QStringLiteral("导入完成：新增 %1 门课程、%2 条上课安排")
        .arg(result.importedCourses)
        .arg(result.importedMeetings));
    return true;
}

bool ScheduleManagementViewModel::savePeriodDefinition(
    int periodNumber,
    const QString &name,
    const QString &startTime,
    const QString &endTime)
{
    clearMessages();
    if (m_periodDefinitionService == nullptr) {
        setError(QStringLiteral("当前构建未启用节次时间设置"));
        return false;
    }

    const PeriodDefinitionError result = m_periodDefinitionService->save(
        PeriodDefinition{
            .periodNumber = periodNumber,
            .name = name,
            .startTime = startTime,
            .endTime = endTime,
        });
    if (result != PeriodDefinitionError::None) {
        setError(result == PeriodDefinitionError::InvalidInput
            ? QStringLiteral("请检查节次名称和 HH:mm 格式的起止时间")
            : QStringLiteral("无法保存节次时间设置"));
        return false;
    }

    const LoadPeriodDefinitionsResult definitions =
        m_periodDefinitionService->load();
    if (std::holds_alternative<PeriodDefinitionError>(definitions)) {
        setError(QStringLiteral("节次已保存，但无法重新读取设置"));
        return false;
    }
    m_periodDefinitions =
        std::get<std::vector<PeriodDefinition>>(definitions);
    emit periodDefinitionsChanged();
    return true;
}

void ScheduleManagementViewModel::clearMessages()
{
    clearError();
    clearConflicts();
    setImportMessage({});
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
    rebuildTermScheduleModel();
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

void ScheduleManagementViewModel::rebuildTermScheduleModel()
{
    std::vector<WeeklyScheduleItem> items;
    for (const CourseMeeting &meeting : m_meetings) {
        const Course *course = findCourse(meeting.courseId());
        if (course == nullptr || course->termId() != m_selectedTermId) {
            continue;
        }
        items.push_back(WeeklyScheduleItem{
            .courseId = course->id(),
            .meetingId = meeting.id(),
            .courseName = course->name(),
            .teacher = course->teacher(),
            .location = course->location(),
            .color = course->color(),
            .weekday = meeting.weekday(),
            .startPeriod = meeting.startPeriod(),
            .endPeriod = meeting.endPeriod(),
            .startWeek = meeting.startWeek(),
            .endWeek = meeting.endWeek(),
            .weekPattern = meeting.weekPattern(),
            .activeWeeks = meeting.activeWeeks(),
        });
    }
    std::ranges::sort(
        items,
        [](const WeeklyScheduleItem &left,
           const WeeklyScheduleItem &right) {
            if (left.weekday != right.weekday) {
                return static_cast<int>(left.weekday)
                    < static_cast<int>(right.weekday);
            }
            if (left.startPeriod != right.startPeriod) {
                return left.startPeriod < right.startPeriod;
            }
            if (left.endPeriod != right.endPeriod) {
                return left.endPeriod < right.endPeriod;
            }
            return left.courseName < right.courseName;
        });
    m_termScheduleModel.replaceItems(std::move(items));
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

void ScheduleManagementViewModel::setImportMessage(QString message)
{
    if (m_importMessage == message) {
        return;
    }
    m_importMessage = std::move(message);
    emit importMessageChanged();
}

} // namespace stukit::schedule::presentation
