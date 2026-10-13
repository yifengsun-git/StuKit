#pragma once

#include "IPeriodDefinitionService.h"

#include <QSqlDatabase>

namespace stukit::schedule {

class SqlitePeriodDefinitionService final
    : public IPeriodDefinitionService
{
public:
    explicit SqlitePeriodDefinitionService(QSqlDatabase database);

    [[nodiscard]] LoadPeriodDefinitionsResult load() const override;
    [[nodiscard]] PeriodDefinitionError save(
        PeriodDefinition definition) override;

private:
    QSqlDatabase m_database;
};

} // namespace stukit::schedule
