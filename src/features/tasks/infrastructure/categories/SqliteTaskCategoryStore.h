#pragma once

#include "ITaskCategoryStore.h"

#include <QSqlDatabase>

namespace stukit::tasks {

class SqliteTaskCategoryStore final : public ITaskCategoryStore
{
public:
    explicit SqliteTaskCategoryStore(QSqlDatabase database);

    [[nodiscard]] std::optional<QStringList> categories() const override;
    [[nodiscard]] bool addCategory(const QString &name) override;

private:
    QSqlDatabase m_database;
};

} // namespace stukit::tasks
