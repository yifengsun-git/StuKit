#include "SqliteTaskRepository.h"
#include "SqliteTaskCategoryStore.h"
#include "database/SqliteDatabase.h"

#include <QCoreApplication>
#include <QDir>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

using stukit::accounting::SqliteDatabase;
using namespace stukit::tasks;

namespace {

QString connectionName()
{
    return QStringLiteral("stukit-task-test-%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QTemporaryDir temporaryDirectory()
{
    return QTemporaryDir(QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("task-sqlite-test-XXXXXX")));
}

QDateTime utc(int day, int hour = 8)
{
    return QDateTime(QDate(2026, 10, day), QTime(hour, 0), QTimeZone::UTC);
}

TaskItem task(
    const QUuid &id,
    QString title = QStringLiteral("完成高数作业"),
    TaskStatus status = TaskStatus::Pending)
{
    const auto courseId = QUuid::createUuid();
    return *TaskItem::create(TaskItemData{
        .id = id,
        .title = std::move(title),
        .description = QStringLiteral("习题 1～10"),
        .dueAtUtc = utc(20),
        .reminderAtUtc = utc(19),
        .priority = TaskPriority::High,
        .categoryName = QStringLiteral("作业"),
        .status = status,
        .courseId = courseId,
        .courseNameSnapshot = QStringLiteral("高等数学"),
        .completedAtUtc = status == TaskStatus::Pending
            ? std::nullopt : std::optional<QDateTime>(utc(18)),
        .createdAtUtc = utc(13),
    });
}

} // namespace

class SqliteTaskRepositoryTest final : public QObject
{
    Q_OBJECT

private slots:
    void migrationCreatesVersionEightSchema();
    void savesFindsUpdatesListsAndRemoves();
    void roundTripsOptionalFieldsAndArchivedStatus();
    void persistsCustomCategories();
};

void SqliteTaskRepositoryTest::migrationCreatesVersionEightSchema()
{
    QTemporaryDir directory = temporaryDirectory();
    QVERIFY(directory.isValid());
    SqliteDatabase database(connectionName());
    QVERIFY2(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))),
             qPrintable(database.lastError()));
    QVERIFY2(database.migrate(), qPrintable(database.lastError()));
    QCOMPARE(SqliteDatabase::CurrentSchemaVersion, 10);

    QSqlQuery query(database.connection());
    QVERIFY(query.exec(QStringLiteral(
        "SELECT (SELECT COUNT(*) FROM schema_migrations WHERE version = 10), "
        "(SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' "
        "AND name = 'tasks'), "
        "(SELECT COUNT(*) FROM sqlite_master WHERE type = 'index' "
        "AND name IN ('idx_tasks_status_due_at', 'idx_tasks_category', "
        "'idx_tasks_course_id'))")));
    QVERIFY(query.next());
    QCOMPARE(query.value(0).toInt(), 1);
    QCOMPARE(query.value(1).toInt(), 1);
    QCOMPARE(query.value(2).toInt(), 3);
}

void SqliteTaskRepositoryTest::savesFindsUpdatesListsAndRemoves()
{
    QTemporaryDir directory = temporaryDirectory();
    QVERIFY(directory.isValid());
    SqliteDatabase database(connectionName());
    QVERIFY(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))));
    QVERIFY(database.migrate());
    SqliteTaskRepository repository(database.connection());
    const QUuid id = QUuid::createUuid();

    QVERIFY(repository.save(task(id)));
    auto restored = repository.findById(id);
    QVERIFY(restored.has_value());
    QCOMPARE(restored->title(), QStringLiteral("完成高数作业"));
    QCOMPARE(restored->dueAtUtc(), std::optional<QDateTime>(utc(20)));
    QVERIFY(restored->courseId().has_value());

    QVERIFY(repository.save(task(id, QStringLiteral("完成并订正高数作业"))));
    restored = repository.findById(id);
    QVERIFY(restored.has_value());
    QCOMPARE(restored->title(), QStringLiteral("完成并订正高数作业"));
    const auto all = repository.findAll();
    QVERIFY(all.has_value());
    QCOMPARE(all->size(), std::size_t{1});

    QVERIFY(repository.remove(id));
    QVERIFY(!repository.findById(id).has_value());
    QVERIFY(!repository.remove(id));
}

void SqliteTaskRepositoryTest::roundTripsOptionalFieldsAndArchivedStatus()
{
    QTemporaryDir directory = temporaryDirectory();
    QVERIFY(directory.isValid());
    SqliteDatabase database(connectionName());
    QVERIFY(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))));
    QVERIFY(database.migrate());
    SqliteTaskRepository repository(database.connection());

    auto data = task(QUuid::createUuid(), QStringLiteral("竞赛报名"),
                     TaskStatus::Archived).data();
    data.dueAtUtc.reset();
    data.reminderAtUtc.reset();
    data.courseId.reset();
    data.courseNameSnapshot.clear();
    data.categoryName = QStringLiteral("社团");
    const auto standalone = TaskItem::create(data);
    QVERIFY(standalone.has_value());
    QVERIFY2(repository.save(*standalone), qPrintable(repository.lastError()));

    const auto restored = repository.findById(standalone->id());
    QVERIFY(restored.has_value());
    QCOMPARE(restored->status(), TaskStatus::Archived);
    QCOMPARE(restored->categoryName(), QStringLiteral("社团"));
    QVERIFY(!restored->dueAtUtc().has_value());
    QVERIFY(!restored->courseId().has_value());
    QCOMPARE(restored->completedAtUtc(),
             std::optional<QDateTime>(utc(18)));
}

void SqliteTaskRepositoryTest::persistsCustomCategories()
{
    QTemporaryDir directory = temporaryDirectory();
    QVERIFY(directory.isValid());
    SqliteDatabase database(connectionName());
    QVERIFY(database.open(directory.filePath(QStringLiteral("stukit.sqlite"))));
    QVERIFY(database.migrate());
    SqliteTaskCategoryStore store(database.connection());

    const auto defaults = store.categories();
    QVERIFY(defaults.has_value());
    QCOMPARE(*defaults, QStringList({QStringLiteral("作业"),
                                     QStringLiteral("考试"),
                                     QStringLiteral("竞赛")}));
    QVERIFY(store.addCategory(QStringLiteral("社团")));
    QVERIFY(!store.addCategory(QStringLiteral("社团")));
    const auto restored = store.categories();
    QVERIFY(restored.has_value());
    QVERIFY(restored->contains(QStringLiteral("社团")));
}

QTEST_GUILESS_MAIN(SqliteTaskRepositoryTest)

#include "SqliteTaskRepositoryTest.moc"
