#pragma once

#include "IPlanRevisionRepository.h"

#include <QSqlDatabase>

namespace stukit::planning {

class SqlitePlanRevisionRepository final
    : public IPlanRevisionRepository
{
public:
    explicit SqlitePlanRevisionRepository(QSqlDatabase database);

    bool save(const PlanRevision &revision, QString &error) override;
    std::vector<PlanRevision> forProject(
        const QUuid &projectId, QString &error) const override;
    std::optional<PlanRevision> latest(
        const QUuid &projectId, QString &error) const override;

private:
    std::optional<PlanRevision> revisionFromCurrentRow(
        const class QSqlQuery &query, QString &error) const;

    QSqlDatabase m_database;
};

} // namespace stukit::planning
