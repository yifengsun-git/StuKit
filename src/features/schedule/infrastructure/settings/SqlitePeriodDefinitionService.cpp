#include "SqlitePeriodDefinitionService.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QTime>

#include <utility>

namespace stukit::schedule {
namespace {

bool isValid(const PeriodDefinition &definition)
{
    const QString name = definition.name.trimmed();
    const QTime start = QTime::fromString(
        definition.startTime.trimmed(), QStringLiteral("HH:mm"));
    const QTime end = QTime::fromString(
        definition.endTime.trimmed(), QStringLiteral("HH:mm"));
    return definition.periodNumber >= 1
        && definition.periodNumber <= 20
        && !name.isEmpty()
        && name.size() <= 20
        && start.isValid()
        && end.isValid()
        && start < end;
}

} // namespace

SqlitePeriodDefinitionService::SqlitePeriodDefinitionService(
    QSqlDatabase database)
    : m_database(std::move(database))
{
}

LoadPeriodDefinitionsResult SqlitePeriodDefinitionService::load() const
{
    if (!m_database.isOpen()) {
        return PeriodDefinitionError::StorageFailure;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT period_number, name, start_time, end_time "
            "FROM schedule_period_definitions ORDER BY period_number"))) {
        return PeriodDefinitionError::StorageFailure;
    }

    std::vector<PeriodDefinition> definitions;
    while (query.next()) {
        definitions.push_back(PeriodDefinition{
            .periodNumber = query.value(0).toInt(),
            .name = query.value(1).toString(),
            .startTime = query.value(2).toString(),
            .endTime = query.value(3).toString(),
        });
    }
    return definitions;
}

PeriodDefinitionError SqlitePeriodDefinitionService::save(
    PeriodDefinition definition)
{
    definition.name = definition.name.trimmed();
    definition.startTime = definition.startTime.trimmed();
    definition.endTime = definition.endTime.trimmed();
    if (!isValid(definition)) {
        return PeriodDefinitionError::InvalidInput;
    }
    if (!m_database.isOpen()) {
        return PeriodDefinitionError::StorageFailure;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO schedule_period_definitions("
        "period_number, name, start_time, end_time) "
        "VALUES(:periodNumber, :name, :startTime, :endTime) "
        "ON CONFLICT(period_number) DO UPDATE SET "
        "name = excluded.name, start_time = excluded.start_time, "
        "end_time = excluded.end_time"));
    query.bindValue(QStringLiteral(":periodNumber"),
                    definition.periodNumber);
    query.bindValue(QStringLiteral(":name"), definition.name);
    query.bindValue(QStringLiteral(":startTime"), definition.startTime);
    query.bindValue(QStringLiteral(":endTime"), definition.endTime);
    return query.exec()
        ? PeriodDefinitionError::None
        : PeriodDefinitionError::StorageFailure;
}

} // namespace stukit::schedule
