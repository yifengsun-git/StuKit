#include "ChangeTaskStatus.h"
#include "DeleteTask.h"
#include "GetTaskOverview.h"
#include "SaveTask.h"
#include "ScanTaskReminders.h"
#include "support/InMemoryTaskRepository.h"

#include <QTest>
#include <QTimeZone>

#include <utility>

using namespace stukit::tasks;
using stukit::tasks::test::InMemoryTaskRepository;

namespace {

QDateTime utc(int day, int hour = 8)
{
    return QDateTime(QDate(2026, 10, day), QTime(hour, 0), QTimeZone::UTC);
}

SaveTaskRequest request(QString title = QStringLiteral("完成高数作业"))
{
    return SaveTaskRequest{
        .id = {},
        .title = std::move(title),
        .description = QStringLiteral("习题 1～10"),
        .dueAtUtc = utc(20),
        .reminderAtUtc = utc(19),
        .priority = TaskPriority::High,
        .categoryName = QStringLiteral("作业"),
        .courseId = QUuid::createUuid(),
        .courseNameSnapshot = QStringLiteral("高等数学"),
        .nowUtc = utc(13),
    };
}

TaskItem savedTask(SaveTask &save, SaveTaskRequest input = request())
{
    auto result = save.execute(input);
    Q_ASSERT(std::holds_alternative<TaskItem>(result));
    return std::get<TaskItem>(std::move(result));
}

class FakeCourseLookup final : public ICourseLookup
{
public:
    FindCourseResult findCourse(const QUuid &id) const override
    {
        if (fail) {
            return CourseLookupError::StorageFailure;
        }
        if (reference.id != id) {
            return CourseLookupError::NotFound;
        }
        return reference;
    }

    AvailableCoursesResult availableCourses() const override
    {
        return std::vector<CourseReference>{reference};
    }

    CourseReference reference;
    bool fail{false};
};

class FakeDeliveryStore final : public IReminderDeliveryStore
{
public:
    bool wasDelivered(const QUuid &id, const QDateTime &at) const override
    {
        return taskId == id && reminderAt == at;
    }
    bool markDelivered(
        const QUuid &id, const QDateTime &at, const QDateTime &) override
    {
        taskId = id;
        reminderAt = at;
        return true;
    }
    QUuid taskId;
    QDateTime reminderAt;
};

class FakeNotifier final : public ITaskReminderNotifier
{
public:
    bool deliver(const TaskReminder &value) override
    {
        reminders.push_back(value);
        return true;
    }
    std::vector<TaskReminder> reminders;
};

} // namespace

class TaskApplicationTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndUpdatesTask();
    void resolvesCourseSnapshotThroughLookup();
    void completesArchivesRestoresAndReopens();
    void rejectsInvalidStatusTransitions();
    void filtersSortsAndCalculatesOverview();
    void deletesTaskAndReportsErrors();
    void deliversDueReminderOnlyOnce();
};

void TaskApplicationTest::createsAndUpdatesTask()
{
    InMemoryTaskRepository repository;
    SaveTask save(repository);

    TaskItem task = savedTask(save);
    QVERIFY(!task.id().isNull());
    QCOMPARE(repository.tasks.size(), std::size_t{1});

    auto update = request(QStringLiteral("高数作业（订正）"));
    update.id = task.id();
    update.nowUtc = utc(14);
    const auto updatedResult = save.execute(update);

    QVERIFY(std::holds_alternative<TaskItem>(updatedResult));
    const auto &updated = std::get<TaskItem>(updatedResult);
    QCOMPARE(updated.id(), task.id());
    QCOMPARE(updated.createdAtUtc(), utc(13));
    QCOMPARE(repository.tasks.size(), std::size_t{1});
}

void TaskApplicationTest::resolvesCourseSnapshotThroughLookup()
{
    InMemoryTaskRepository repository;
    const QUuid courseId = QUuid::createUuid();
    FakeCourseLookup lookup;
    lookup.reference = CourseReference{
        .id = courseId,
        .name = QStringLiteral("数据结构"),
        .termId = QUuid::createUuid(),
        .termName = QStringLiteral("2026 秋季学期"),
    };
    SaveTask save(repository, &lookup);
    auto input = request();
    input.courseId = courseId;
    input.courseNameSnapshot = QStringLiteral("伪造名称");

    const auto result = save.execute(input);

    QVERIFY(std::holds_alternative<TaskItem>(result));
    QCOMPARE(std::get<TaskItem>(result).courseNameSnapshot(),
             QStringLiteral("数据结构"));

    input.id = {};
    input.courseId = QUuid::createUuid();
    const auto missing = save.execute(input);
    QVERIFY(std::holds_alternative<SaveTaskError>(missing));
    QCOMPARE(std::get<SaveTaskError>(missing), SaveTaskError::CourseNotFound);
}

void TaskApplicationTest::completesArchivesRestoresAndReopens()
{
    InMemoryTaskRepository repository;
    SaveTask save(repository);
    const TaskItem task = savedTask(save);
    ChangeTaskStatus change(repository);

    auto completed = change.execute({task.id(), TaskStatusAction::Complete, utc(14)});
    QVERIFY(std::holds_alternative<TaskItem>(completed));
    QCOMPARE(std::get<TaskItem>(completed).status(), TaskStatus::Completed);

    auto archived = change.execute({task.id(), TaskStatusAction::Archive, utc(15)});
    QVERIFY(std::holds_alternative<TaskItem>(archived));
    QCOMPARE(std::get<TaskItem>(archived).status(), TaskStatus::Archived);

    auto restored = change.execute({task.id(), TaskStatusAction::Restore, utc(16)});
    QVERIFY(std::holds_alternative<TaskItem>(restored));
    auto reopened = change.execute({task.id(), TaskStatusAction::Reopen, utc(17)});
    QVERIFY(std::holds_alternative<TaskItem>(reopened));
    QCOMPARE(std::get<TaskItem>(reopened).status(), TaskStatus::Pending);
    QVERIFY(!std::get<TaskItem>(reopened).completedAtUtc().has_value());
}

void TaskApplicationTest::rejectsInvalidStatusTransitions()
{
    InMemoryTaskRepository repository;
    SaveTask save(repository);
    const TaskItem task = savedTask(save);
    ChangeTaskStatus change(repository);

    const auto result = change.execute(
        {task.id(), TaskStatusAction::Archive, utc(14)});

    QVERIFY(std::holds_alternative<ChangeTaskStatusError>(result));
    QCOMPARE(std::get<ChangeTaskStatusError>(result),
             ChangeTaskStatusError::InvalidTransition);
}

void TaskApplicationTest::filtersSortsAndCalculatesOverview()
{
    InMemoryTaskRepository repository;
    SaveTask save(repository);
    TaskItem homework = savedTask(save);

    auto examRequest = request(QStringLiteral("准备英语考试"));
    examRequest.categoryName = QStringLiteral("考试");
    examRequest.courseId.reset();
    examRequest.courseNameSnapshot.clear();
    examRequest.dueAtUtc = utc(18);
    examRequest.reminderAtUtc = utc(17);
    const TaskItem exam = savedTask(save, examRequest);

    ChangeTaskStatus change(repository);
    QVERIFY(std::holds_alternative<TaskItem>(change.execute(
        {homework.id(), TaskStatusAction::Complete, utc(14)})));
    QVERIFY(std::holds_alternative<TaskItem>(change.execute(
        {homework.id(), TaskStatusAction::Archive, utc(15)})));

    GetTaskOverview getOverview(repository);
    const auto result = getOverview.execute({}, utc(21));

    QVERIFY(std::holds_alternative<TaskOverview>(result));
    const auto &overview = std::get<TaskOverview>(result);
    QCOMPARE(overview.tasks.size(), std::size_t{1});
    QCOMPARE(overview.tasks.front().id(), exam.id());
    QCOMPARE(overview.pendingCount, 1);
    QCOMPARE(overview.completedCount, 1);
    QCOMPARE(overview.archivedCount, 1);
    QCOMPARE(overview.overdueCount, 1);
    QCOMPARE(overview.completionRate, 0.5);

    TaskFilter filter;
    filter.includeArchived = true;
    filter.categoryName = QStringLiteral("作业");
    const auto filtered = getOverview.execute(filter, utc(21));
    QVERIFY(std::holds_alternative<TaskOverview>(filtered));
    const auto &homeworkOverview = std::get<TaskOverview>(filtered);
    QCOMPARE(homeworkOverview.tasks.size(), std::size_t{1});
    QCOMPARE(homeworkOverview.completedCount, 1);
    QCOMPARE(homeworkOverview.pendingCount, 0);
    QCOMPARE(homeworkOverview.completionRate, 1.0);
}

void TaskApplicationTest::deletesTaskAndReportsErrors()
{
    InMemoryTaskRepository repository;
    SaveTask save(repository);
    const TaskItem task = savedTask(save);
    DeleteTask remove(repository);

    QVERIFY(!remove.execute(task.id()).has_value());
    QCOMPARE(repository.tasks.size(), std::size_t{0});
    const auto missing = remove.execute(task.id());
    QVERIFY(missing.has_value());
    QCOMPARE(*missing, DeleteTaskError::TaskNotFound);
    const auto invalid = remove.execute({});
    QVERIFY(invalid.has_value());
    QCOMPARE(*invalid, DeleteTaskError::InvalidInput);
}

void TaskApplicationTest::deliversDueReminderOnlyOnce()
{
    InMemoryTaskRepository repository;
    SaveTask save(repository);
    const TaskItem task = savedTask(save);
    FakeDeliveryStore store;
    FakeNotifier notifier;
    ScanTaskReminders scan(repository, store, notifier);

    const auto first = scan.execute(utc(19));
    QVERIFY(std::holds_alternative<int>(first));
    QCOMPARE(std::get<int>(first), 1);
    QCOMPARE(notifier.reminders.size(), std::size_t{1});
    QCOMPARE(notifier.reminders.front().taskId, task.id());

    const auto repeated = scan.execute(utc(20));
    QVERIFY(std::holds_alternative<int>(repeated));
    QCOMPARE(std::get<int>(repeated), 0);
    QCOMPARE(notifier.reminders.size(), std::size_t{1});
}

QTEST_APPLESS_MAIN(TaskApplicationTest)

#include "TaskApplicationTest.moc"
