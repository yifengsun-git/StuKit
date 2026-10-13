#pragma once

#include <QString>

#include <variant>
#include <vector>

namespace stukit::schedule {

struct PeriodDefinition
{
    int periodNumber{0};
    QString name;
    QString startTime;
    QString endTime;
};

enum class PeriodDefinitionError
{
    None,
    InvalidInput,
    StorageFailure,
};

using LoadPeriodDefinitionsResult =
    std::variant<std::vector<PeriodDefinition>, PeriodDefinitionError>;

class IPeriodDefinitionService
{
public:
    virtual ~IPeriodDefinitionService() = default;

    [[nodiscard]] virtual LoadPeriodDefinitionsResult load() const = 0;
    [[nodiscard]] virtual PeriodDefinitionError save(
        PeriodDefinition definition) = 0;
};

} // namespace stukit::schedule
