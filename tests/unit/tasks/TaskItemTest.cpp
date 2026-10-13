#include "TaskItem.h"

#include <QTest>
#include <QTimeZone>

using namespace stukit::tasks;

namespace {

QDateTime utc(int day, int hour = 8)
{
    return QDateTime(QDate(2026, 10, day), QTime(hour, 0), QTimeZone::UTC);
}

TaskItemData validTaskData()
{
    return TaskItemData{
        .id = QUuid::createUuid(),
        .title = QStringLiteral("完成高数作业"),
        .description = QStringLiteral("习题 1～10"),
        .dueAtUtc = utc(20),
        .reminderAtUtc = utc(19),
        .priority = TaskPriority::High,
        .categoryName = QStringLiteral("作业"),
        .status = TaskStatus::Pending,
        .courseId = QUuid::createUuid(),
        .courseNameSnapshot = QStringLiteral("高等数学"),
        .completedAtUtc = std::nullopt,
        .createdAtUtc = utc(13),
    };
}

} // namespace

class TaskItemTest final : public QObject
{
    Q_OBJECT

private slots:
    void createsAndNormalizesValidTask();
    void allowsTaskWithoutDeadlineReminderOrCourse();
    void rejectsInvalidIdentityAndText();
    void rejectsInvalidTimes();
    void enforcesCompletionState();
    void enforcesCourseReferencePair();
    void reportsOverdueAndReminderState();
};

void TaskItemTest::createsAndNormalizesValidTask()
{
    auto data = validTaskData();
    data.title = QStringLiteral("  完成高数作业  ");
    data.description = QStringLiteral("  习题 1～10  ");
    data.courseNameSnapshot = QStringLiteral("  高等数学  ");

    const auto task = TaskItem::create(data);

    QVERIFY(task.has_value());
    QCOMPARE(task->title(), QStringLiteral("完成高数作业"));
    QCOMPARE(task->description(), QStringLiteral("习题 1～10"));
    QCOMPARE(task->categoryName(), QStringLiteral("作业"));
    QCOMPARE(task->courseNameSnapshot(), QStringLiteral("高等数学"));
    QCOMPARE(task->priority(), TaskPriority::High);
    QVERIFY(!task->isCompleted());
}

void TaskItemTest::allowsTaskWithoutDeadlineReminderOrCourse()
{
    auto data = validTaskData();
    data.dueAtUtc.reset();
    data.reminderAtUtc.reset();
    data.courseId.reset();
    data.courseNameSnapshot.clear();

    const auto task = TaskItem::create(data);

    QVERIFY(task.has_value());
    QVERIFY(!task->dueAtUtc().has_value());
    QVERIFY(!task->courseId().has_value());
}

void TaskItemTest::rejectsInvalidIdentityAndText()
{
    auto nullId = validTaskData();
    nullId.id = {};
    QVERIFY(!TaskItem::create(nullId).has_value());

    auto emptyTitle = validTaskData();
    emptyTitle.title = QStringLiteral("   ");
    QVERIFY(!TaskItem::create(emptyTitle).has_value());

    auto longTitle = validTaskData();
    longTitle.title = QString(TaskItem::MaximumTitleLength + 1, QLatin1Char('a'));
    QVERIFY(!TaskItem::create(longTitle).has_value());

    auto longDescription = validTaskData();
    longDescription.description = QString(
        TaskItem::MaximumDescriptionLength + 1, QLatin1Char('a'));
    QVERIFY(!TaskItem::create(longDescription).has_value());

    auto emptyCategory = validTaskData();
    emptyCategory.categoryName = QStringLiteral("  ");
    QVERIFY(!TaskItem::create(emptyCategory).has_value());
}

void TaskItemTest::rejectsInvalidTimes()
{
    auto localDue = validTaskData();
    localDue.dueAtUtc = QDateTime(QDate(2026, 10, 20), QTime(8, 0));
    QVERIFY(!TaskItem::create(localDue).has_value());

    auto reminderWithoutDue = validTaskData();
    reminderWithoutDue.dueAtUtc.reset();
    QVERIFY(!TaskItem::create(reminderWithoutDue).has_value());

    auto lateReminder = validTaskData();
    lateReminder.reminderAtUtc = utc(21);
    QVERIFY(!TaskItem::create(lateReminder).has_value());
}

void TaskItemTest::enforcesCompletionState()
{
    auto pendingWithCompletion = validTaskData();
    pendingWithCompletion.completedAtUtc = utc(15);
    QVERIFY(!TaskItem::create(pendingWithCompletion).has_value());

    auto completedWithoutTime = validTaskData();
    completedWithoutTime.status = TaskStatus::Completed;
    QVERIFY(!TaskItem::create(completedWithoutTime).has_value());

    auto archived = validTaskData();
    archived.status = TaskStatus::Archived;
    archived.completedAtUtc = utc(15);
    const auto task = TaskItem::create(archived);
    QVERIFY(task.has_value());
    QVERIFY(task->isCompleted());
    QVERIFY(task->isArchived());
}

void TaskItemTest::enforcesCourseReferencePair()
{
    auto idWithoutName = validTaskData();
    idWithoutName.courseNameSnapshot = QStringLiteral("  ");
    QVERIFY(!TaskItem::create(idWithoutName).has_value());

    auto nameWithoutId = validTaskData();
    nameWithoutId.courseId.reset();
    QVERIFY(!TaskItem::create(nameWithoutId).has_value());

    auto nullCourseId = validTaskData();
    nullCourseId.courseId = QUuid{};
    QVERIFY(!TaskItem::create(nullCourseId).has_value());
}

void TaskItemTest::reportsOverdueAndReminderState()
{
    const auto task = TaskItem::create(validTaskData());
    QVERIFY(task.has_value());

    QVERIFY(!task->isOverdue(utc(19)));
    QVERIFY(task->reminderIsDue(utc(19)));
    QVERIFY(task->isOverdue(utc(21)));

    auto completedData = validTaskData();
    completedData.status = TaskStatus::Completed;
    completedData.completedAtUtc = utc(18);
    const auto completed = TaskItem::create(completedData);
    QVERIFY(completed.has_value());
    QVERIFY(!completed->isOverdue(utc(21)));
    QVERIFY(!completed->reminderIsDue(utc(21)));
}

QTEST_APPLESS_MAIN(TaskItemTest)

#include "TaskItemTest.moc"
