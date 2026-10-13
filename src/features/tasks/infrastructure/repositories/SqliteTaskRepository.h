#pragma once

#include "ITaskRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::tasks {

class SqliteTaskRepository final : public ITaskRepository
{
public:
    explicit SqliteTaskRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const TaskItem &task) override;
    [[nodiscard]] bool remove(const QUuid &id) override;
    [[nodiscard]] std::optional<TaskItem> findById(
        const QUuid &id) const override;
    [[nodiscard]] std::optional<std::vector<TaskItem>> findAll() const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<TaskItem> taskFromCurrentRow(
        const QSqlQuery &query) const;
    [[nodiscard]] std::optional<std::vector<TaskItem>> tasksFromQuery(
        QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::tasks
