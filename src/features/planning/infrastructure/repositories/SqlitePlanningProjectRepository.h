#pragma once

#include "IPlanningProjectRepository.h"

#include <QSqlDatabase>

namespace stukit::planning {

class SqlitePlanningProjectRepository final
    : public IPlanningProjectRepository
{
public:
    explicit SqlitePlanningProjectRepository(QSqlDatabase database);

    bool save(const PlanningProject &project, QString &error) override;
    std::vector<PlanningProject> all(QString &error) const override;
    std::optional<PlanningProject> findById(
        const QUuid &id, QString &error) const override;

private:
    std::optional<PlanningProject> projectFromCurrentRow(
        const class QSqlQuery &query, QString &error) const;

    QSqlDatabase m_database;
};

} // namespace stukit::planning
