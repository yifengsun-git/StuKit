#include "ChangeTaskStatus.h"
#include "DeleteTask.h"
#include "GetTaskOverview.h"
#include "ICourseLookup.h"
#include "SaveTask.h"
#include "TaskReminder.h"
#include "TasksViewModel.h"
#include "support/InMemoryTaskRepository.h"

#include <QCoreApplication>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QTimeZone>
#include <QVariantMap>

#include <memory>

using namespace stukit::tasks;
using namespace stukit::tasks::presentation;
using stukit::tasks::test::InMemoryTaskRepository;

namespace {

class EmptyCourseLookup final : public ICourseLookup
{
public:
    FindCourseResult findCourse(const QUuid &) const override
    {
        return CourseLookupError::NotFound;
    }

    AvailableCoursesResult availableCourses() const override
    {
        return std::vector<CourseReference>{};
    }
};

class InMemoryCategoryStore final : public ITaskCategoryStore
{
public:
    std::optional<QStringList> categories() const override { return names; }
    bool addCategory(const QString &name) override
    {
        if (names.contains(name, Qt::CaseInsensitive)) return false;
        names.push_back(name);
        return true;
    }
    QStringList names{QStringLiteral("作业"), QStringLiteral("考试"),
                      QStringLiteral("竞赛")};
};

struct Fixture
{
    Fixture()
        : save(repository, &courses)
        , change(repository)
        , remove(repository)
        , overview(repository)
        , viewModel(save, change, remove, overview, courses, categories,
                    QTimeZone(QByteArrayLiteral("UTC+08:00")))
    {
        viewModel.load();
    }

    InMemoryTaskRepository repository;
    EmptyCourseLookup courses;
    InMemoryCategoryStore categories;
    SaveTask save;
    ChangeTaskStatus change;
    DeleteTask remove;
    GetTaskOverview overview;
    TasksViewModel viewModel;
};

std::unique_ptr<QObject> createTasksPage(
    QQmlEngine &engine, TasksViewModel &viewModel, qreal width)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitTasksTest"), QStringLiteral("TasksHomePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }
    std::unique_ptr<QObject> page(component.createWithInitialProperties({
        {QStringLiteral("viewModel"),
         QVariant::fromValue(static_cast<QObject *>(&viewModel))},
        {QStringLiteral("width"), width},
        {QStringLiteral("height"), 720.0},
    }));
    if (!page) {
        qWarning().noquote() << component.errorString();
        return {};
    }
    QCoreApplication::processEvents();
    return page;
}

} // namespace

class TasksQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void rendersTaskAndResponsiveLayout();
    void rendersAndDismissesReminder();
    void usesScrollableDateTimePicker();
};

void TasksQmlIntegrationTest::rendersTaskAndResponsiveLayout()
{
    Fixture fixture;
    QQmlEngine engine;
    auto page = createTasksPage(engine, fixture.viewModel, 360);
    QVERIFY(page);

    QObject *stats = page->findChild<QObject *>(QStringLiteral("taskStatsGrid"));
    QObject *empty = page->findChild<QObject *>(QStringLiteral("taskEmptyState"));
    QObject *tasks = page->findChild<QObject *>(QStringLiteral("taskListRepeater"));
    QVERIFY(stats);
    QVERIFY(empty);
    QVERIFY(tasks);
    QVERIFY(page->property("compact").toBool());
    QCOMPARE(stats->property("columns").toInt(), 2);
    QVERIFY(empty->property("visible").toBool());
    QCOMPARE(tasks->property("count").toInt(), 0);

    QVERIFY(fixture.viewModel.addCategory(QStringLiteral("社团")));
    QVERIFY(fixture.viewModel.categoryOptions().contains(QStringLiteral("社团")));
    QVERIFY2(fixture.viewModel.save(
                 {}, QStringLiteral("准备数据库考试"),
                 QStringLiteral("复习事务与索引"),
                 QStringLiteral("2099-01-03 18:00"),
                 QStringLiteral("2099-01-03 17:00"),
                 3, QStringLiteral("社团"), {}),
             qPrintable(fixture.viewModel.errorMessage()));
    QCoreApplication::processEvents();
    QCOMPARE(tasks->property("count").toInt(), 1);
    QVERIFY(!empty->property("visible").toBool());
    QCOMPARE(fixture.viewModel.tasks().constFirst().toMap()
                 .value(QStringLiteral("groupLabel")).toString(),
             QStringLiteral("更晚"));
    QCOMPARE(fixture.viewModel.tasks().constFirst().toMap()
                 .value(QStringLiteral("categoryText")).toString(),
             QStringLiteral("社团"));

    page->setProperty("width", 960.0);
    QTRY_VERIFY(!page->property("compact").toBool());
    QTRY_COMPARE(stats->property("columns").toInt(), 4);
}

void TasksQmlIntegrationTest::rendersAndDismissesReminder()
{
    Fixture fixture;
    QQmlEngine engine;
    auto page = createTasksPage(engine, fixture.viewModel, 960);
    QVERIFY(page);
    QObject *reminders = page->findChild<QObject *>(
        QStringLiteral("taskReminderRepeater"));
    QVERIFY(reminders);
    QCOMPARE(reminders->property("count").toInt(), 0);

    QVERIFY(fixture.viewModel.deliver(TaskReminder{
        .taskId = QUuid::createUuid(),
        .title = QStringLiteral("提交课程设计"),
        .courseName = QStringLiteral("软件工程"),
        .dueAtUtc = QDateTime(
            QDate(2099, 1, 5), QTime(10, 0), QTimeZone::UTC),
        .reminderAtUtc = QDateTime(
            QDate(2099, 1, 5), QTime(9, 0), QTimeZone::UTC),
    }));
    QCoreApplication::processEvents();
    QCOMPARE(reminders->property("count").toInt(), 1);

    fixture.viewModel.dismissReminder(0);
    QCoreApplication::processEvents();
    QCOMPARE(reminders->property("count").toInt(), 0);
}

void TasksQmlIntegrationTest::usesScrollableDateTimePicker()
{
    Fixture fixture;
    QQmlEngine engine;
    auto page = createTasksPage(engine, fixture.viewModel, 960);
    QVERIFY(page);
    QObject *picker = page->findChild<QObject *>(
        QStringLiteral("taskDuePicker"));
    QVERIFY(picker);
    QVERIFY(QMetaObject::invokeMethod(
        picker, "openFor",
        Q_ARG(QVariant, QVariant(QStringLiteral("2030-06-15 09:30")))));
    QCoreApplication::processEvents();
    QVariant selected;
    QVERIFY(QMetaObject::invokeMethod(
        picker, "selectedValue", Q_RETURN_ARG(QVariant, selected)));
    QCOMPARE(selected.toString(), QStringLiteral("2030-06-15 09:30"));
    QVERIFY(QMetaObject::invokeMethod(picker, "close"));
}

QTEST_MAIN(TasksQmlIntegrationTest)

#include "TasksQmlIntegrationTest.moc"
