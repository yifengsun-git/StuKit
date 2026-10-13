#include "CsvScheduleImportService.h"

#include "ICourseMeetingRepository.h"
#include "ICourseRepository.h"
#include "SaveCourse.h"
#include "SaveCourseMeeting.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QHash>
#include <QRegularExpression>

#include <algorithm>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace stukit::schedule {
namespace {

struct ImportRow
{
    int rowNumber{0};
    QString courseName;
    QString teacher;
    QString location;
    QString color;
    QString assessmentStandard;
    QString teachingPlan;
    Weekday weekday{Weekday::Monday};
    int startPeriod{0};
    int endPeriod{0};
    std::vector<int> activeWeeks;
};

using CsvTable = std::vector<std::vector<QString>>;

CsvTable parseCsv(const QString &content)
{
    CsvTable table;
    std::vector<QString> row;
    QString field;
    bool quoted = false;

    const auto finishField = [&]() {
        row.push_back(field);
        field.clear();
    };
    const auto finishRow = [&]() {
        finishField();
        table.push_back(row);
        row.clear();
    };

    for (qsizetype index = 0; index < content.size(); ++index) {
        const QChar character = content.at(index);
        if (quoted) {
            if (character == u'"') {
                if (index + 1 < content.size()
                    && content.at(index + 1) == u'"') {
                    field.append(u'"');
                    ++index;
                } else {
                    quoted = false;
                }
            } else {
                field.append(character);
            }
            continue;
        }

        if (character == u'"' && field.isEmpty()) {
            quoted = true;
        } else if (character == u',') {
            finishField();
        } else if (character == u'\n') {
            finishRow();
        } else if (character != u'\r') {
            field.append(character);
        }
    }
    if (!field.isEmpty() || !row.empty()) {
        finishRow();
    }
    return table;
}

QString normalizedHeader(QString value)
{
    value = value.trimmed().toCaseFolded();
    if (!value.isEmpty() && value.front() == QChar::ByteOrderMark) {
        value.removeFirst();
    }
    return value;
}

int findColumn(
    const std::vector<QString> &header,
    std::initializer_list<QString> names)
{
    for (qsizetype index = 0;
         index < static_cast<qsizetype>(header.size());
         ++index) {
        const QString candidate = normalizedHeader(
            header.at(static_cast<std::size_t>(index)));
        for (const QString &name : names) {
            if (candidate == normalizedHeader(name)) {
                return static_cast<int>(index);
            }
        }
    }
    return -1;
}

QString valueAt(const std::vector<QString> &row, int column)
{
    if (column < 0 || column >= static_cast<int>(row.size())) {
        return {};
    }
    return row.at(static_cast<std::size_t>(column)).trimmed();
}

std::optional<Weekday> parseWeekday(QString value)
{
    value = value.trimmed();
    static const QHash<QString, Weekday> weekdays{
        {QStringLiteral("1"), Weekday::Monday},
        {QStringLiteral("周一"), Weekday::Monday},
        {QStringLiteral("星期一"), Weekday::Monday},
        {QStringLiteral("2"), Weekday::Tuesday},
        {QStringLiteral("周二"), Weekday::Tuesday},
        {QStringLiteral("星期二"), Weekday::Tuesday},
        {QStringLiteral("3"), Weekday::Wednesday},
        {QStringLiteral("周三"), Weekday::Wednesday},
        {QStringLiteral("星期三"), Weekday::Wednesday},
        {QStringLiteral("4"), Weekday::Thursday},
        {QStringLiteral("周四"), Weekday::Thursday},
        {QStringLiteral("星期四"), Weekday::Thursday},
        {QStringLiteral("5"), Weekday::Friday},
        {QStringLiteral("周五"), Weekday::Friday},
        {QStringLiteral("星期五"), Weekday::Friday},
        {QStringLiteral("6"), Weekday::Saturday},
        {QStringLiteral("周六"), Weekday::Saturday},
        {QStringLiteral("星期六"), Weekday::Saturday},
        {QStringLiteral("7"), Weekday::Sunday},
        {QStringLiteral("周日"), Weekday::Sunday},
        {QStringLiteral("星期日"), Weekday::Sunday},
        {QStringLiteral("周天"), Weekday::Sunday},
    };
    const auto found = weekdays.constFind(value);
    return found == weekdays.cend()
        ? std::nullopt
        : std::optional<Weekday>(*found);
}

std::optional<std::vector<int>> parseWeeks(const QString &value)
{
    static const QRegularExpression separators(
        QStringLiteral("[;,，、\\s]+"));
    static const QRegularExpression rangeExpression(
        QStringLiteral("^(\\d+)\\s*[-~～]\\s*(\\d+)$"));

    std::vector<int> weeks;
    const QStringList parts = value.trimmed().split(
        separators, Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QRegularExpressionMatch range = rangeExpression.match(part);
        if (range.hasMatch()) {
            const int first = range.captured(1).toInt();
            const int last = range.captured(2).toInt();
            if (first < CourseMeeting::MinimumWeek
                || last > CourseMeeting::MaximumWeek
                || first > last) {
                return std::nullopt;
            }
            for (int week = first; week <= last; ++week) {
                weeks.push_back(week);
            }
            continue;
        }

        bool valid = false;
        const int week = part.toInt(&valid);
        if (!valid
            || week < CourseMeeting::MinimumWeek
            || week > CourseMeeting::MaximumWeek) {
            return std::nullopt;
        }
        weeks.push_back(week);
    }
    std::ranges::sort(weeks);
    const auto uniqueEnd = std::ranges::unique(weeks).begin();
    weeks.erase(uniqueEnd, weeks.end());
    return weeks.empty()
        ? std::nullopt
        : std::optional<std::vector<int>>(std::move(weeks));
}

ScheduleImportResult rowError(int rowNumber, const QString &detail)
{
    return ScheduleImportResult{
        .error = ScheduleImportError::InvalidRow,
        .rowNumber = rowNumber,
        .detail = detail,
    };
}

} // namespace

CsvScheduleImportService::CsvScheduleImportService(
    ICourseRepository &courseRepository,
    ICourseMeetingRepository &meetingRepository,
    SaveCourse &saveCourse,
    SaveCourseMeeting &saveMeeting)
    : m_courseRepository(courseRepository)
    , m_meetingRepository(meetingRepository)
    , m_saveCourse(saveCourse)
    , m_saveMeeting(saveMeeting)
{
}

ScheduleImportResult CsvScheduleImportService::importCsv(
    const QString &filePath,
    const QUuid &termId)
{
    const QString normalizedPath = filePath.trimmed();
    const bool contentUrl = QUrl(normalizedPath).scheme().compare(
        QStringLiteral("content"), Qt::CaseInsensitive) == 0;
    if (termId.isNull() || normalizedPath.isEmpty()
        || (!contentUrl && !QFileInfo(normalizedPath).isFile())) {
        return {.error = ScheduleImportError::InvalidPath};
    }

    QFile file(normalizedPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {.error = ScheduleImportError::FileOpenFailed};
    }
    const CsvTable table = parseCsv(QString::fromUtf8(file.readAll()));
    if (table.empty()) {
        return {.error = ScheduleImportError::InvalidHeader};
    }

    const auto &header = table.front();
    const int nameColumn = findColumn(
        header, {QStringLiteral("课程名称"), QStringLiteral("course_name")});
    const int teacherColumn = findColumn(
        header, {QStringLiteral("教师"), QStringLiteral("teacher")});
    const int locationColumn = findColumn(
        header, {QStringLiteral("教室"), QStringLiteral("location")});
    const int colorColumn = findColumn(
        header, {QStringLiteral("颜色"), QStringLiteral("color")});
    const int weekdayColumn = findColumn(
        header, {QStringLiteral("星期"), QStringLiteral("weekday")});
    const int startColumn = findColumn(
        header, {QStringLiteral("开始节次"), QStringLiteral("start_period")});
    const int endColumn = findColumn(
        header, {QStringLiteral("结束节次"), QStringLiteral("end_period")});
    const int weeksColumn = findColumn(
        header, {QStringLiteral("教学周"), QStringLiteral("weeks")});
    const int assessmentColumn = findColumn(
        header,
        {QStringLiteral("考核标准"), QStringLiteral("assessment")});
    const int planColumn = findColumn(
        header,
        {QStringLiteral("教学安排"), QStringLiteral("teaching_plan")});
    if (nameColumn < 0 || weekdayColumn < 0 || startColumn < 0
        || endColumn < 0 || weeksColumn < 0) {
        return {.error = ScheduleImportError::InvalidHeader};
    }

    std::vector<ImportRow> rows;
    for (std::size_t index = 1; index < table.size(); ++index) {
        const auto &source = table.at(index);
        const int rowNumber = static_cast<int>(index) + 1;
        const bool emptyRow = std::ranges::all_of(
            source, [](const QString &value) { return value.trimmed().isEmpty(); });
        if (emptyRow) {
            continue;
        }

        bool validStart = false;
        bool validEnd = false;
        const int startPeriod = valueAt(source, startColumn).toInt(&validStart);
        const int endPeriod = valueAt(source, endColumn).toInt(&validEnd);
        const auto weekday = parseWeekday(valueAt(source, weekdayColumn));
        const auto weeks = parseWeeks(valueAt(source, weeksColumn));
        const QString courseName = valueAt(source, nameColumn);
        if (courseName.isEmpty()) {
            return rowError(rowNumber, QStringLiteral("课程名称不能为空"));
        }
        if (!weekday.has_value()) {
            return rowError(rowNumber, QStringLiteral("星期格式无效"));
        }
        if (!validStart || !validEnd
            || startPeriod < CourseMeeting::MinimumPeriod
            || endPeriod > CourseMeeting::MaximumPeriod
            || startPeriod > endPeriod) {
            return rowError(rowNumber, QStringLiteral("节次范围无效"));
        }
        if (!weeks.has_value()) {
            return rowError(rowNumber, QStringLiteral("教学周格式无效"));
        }
        rows.push_back(ImportRow{
            .rowNumber = rowNumber,
            .courseName = courseName,
            .teacher = valueAt(source, teacherColumn),
            .location = valueAt(source, locationColumn),
            .color = valueAt(source, colorColumn).isEmpty()
                ? QStringLiteral("#3B82F6")
                : valueAt(source, colorColumn),
            .assessmentStandard = valueAt(source, assessmentColumn),
            .teachingPlan = valueAt(source, planColumn),
            .weekday = *weekday,
            .startPeriod = startPeriod,
            .endPeriod = endPeriod,
            .activeWeeks = *weeks,
        });
    }
    if (rows.empty()) {
        return rowError(0, QStringLiteral("CSV 中没有可导入的课程安排"));
    }

    const auto existingCourses = m_courseRepository.findAll();
    if (!existingCourses.has_value()) {
        return {.error = ScheduleImportError::StorageFailure};
    }

    QHash<QString, QUuid> courseIds;
    for (const Course &course : *existingCourses) {
        if (course.termId() == termId) {
            courseIds.insert(course.name().trimmed().toCaseFolded(), course.id());
        }
    }

    ScheduleImportResult result;
    std::vector<QUuid> createdCourseIds;
    std::vector<QUuid> createdMeetingIds;
    const auto fail = [this, &createdCourseIds, &createdMeetingIds](
                          ScheduleImportResult failure) {
        bool rollbackSucceeded = true;
        for (auto item = createdMeetingIds.crbegin();
             item != createdMeetingIds.crend();
             ++item) {
            rollbackSucceeded = m_meetingRepository.remove(*item)
                && rollbackSucceeded;
        }
        for (auto item = createdCourseIds.crbegin();
             item != createdCourseIds.crend();
             ++item) {
            rollbackSucceeded = m_courseRepository.remove(*item)
                && rollbackSucceeded;
        }
        if (!rollbackSucceeded) {
            failure.error = ScheduleImportError::StorageFailure;
            failure.detail = QStringLiteral("导入失败，且无法完整撤销已写入的数据");
        }
        return failure;
    };
    for (const ImportRow &row : rows) {
        const QString courseKey = row.courseName.trimmed().toCaseFolded();
        QUuid courseId = courseIds.value(courseKey);
        if (courseId.isNull()) {
            const SaveCourseResult savedCourse = m_saveCourse.execute(
                SaveCourseRequest{
                    .id = {},
                    .termId = termId,
                    .name = row.courseName,
                    .teacher = row.teacher,
                    .location = row.location,
                    .assessmentStandard = row.assessmentStandard,
                    .teachingPlan = row.teachingPlan,
                    .color = row.color,
                });
            if (std::holds_alternative<SaveCourseError>(savedCourse)) {
                const SaveCourseError error = std::get<SaveCourseError>(savedCourse);
                return fail({
                    .error = error == SaveCourseError::TermNotFound
                        ? ScheduleImportError::TermNotFound
                        : error == SaveCourseError::TermArchived
                            ? ScheduleImportError::TermArchived
                            : error == SaveCourseError::StorageFailure
                                ? ScheduleImportError::StorageFailure
                                : ScheduleImportError::InvalidRow,
                    .rowNumber = row.rowNumber,
                    .detail = QStringLiteral("课程资料无效"),
                });
            }
            courseId = std::get<Course>(savedCourse).id();
            createdCourseIds.push_back(courseId);
            courseIds.insert(courseKey, courseId);
            ++result.importedCourses;
        }

        const SaveCourseMeetingResult savedMeeting = m_saveMeeting.execute(
            SaveCourseMeetingRequest{
                .id = {},
                .courseId = courseId,
                .weekday = row.weekday,
                .startPeriod = row.startPeriod,
                .endPeriod = row.endPeriod,
                .startWeek = row.activeWeeks.front(),
                .endWeek = row.activeWeeks.back(),
                .weekPattern = WeekPattern::EveryWeek,
                .activeWeeks = row.activeWeeks,
            });
        if (std::holds_alternative<CourseMeetingConflicts>(savedMeeting)) {
            return fail({
                .error = ScheduleImportError::Conflict,
                .rowNumber = row.rowNumber,
                .detail = QStringLiteral("课程时间与现有安排冲突"),
            });
        }
        if (std::holds_alternative<SaveCourseMeetingError>(savedMeeting)) {
            const SaveCourseMeetingError error =
                std::get<SaveCourseMeetingError>(savedMeeting);
            return fail({
                .error = error == SaveCourseMeetingError::TermNotFound
                    ? ScheduleImportError::TermNotFound
                    : error == SaveCourseMeetingError::TermArchived
                        ? ScheduleImportError::TermArchived
                        : error == SaveCourseMeetingError::StorageFailure
                            ? ScheduleImportError::StorageFailure
                            : ScheduleImportError::InvalidRow,
                .rowNumber = row.rowNumber,
                .detail = error == SaveCourseMeetingError::MeetingExceedsTerm
                    ? QStringLiteral("教学周超过学期总周数")
                    : QStringLiteral("课程安排无效"),
            });
        }
        createdMeetingIds.push_back(
            std::get<CourseMeeting>(savedMeeting).id());
        ++result.importedMeetings;
    }

    return result;
}

} // namespace stukit::schedule
