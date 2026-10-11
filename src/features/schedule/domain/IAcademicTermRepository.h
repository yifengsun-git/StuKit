#pragma once

#include "AcademicTerm.h"

#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::schedule {

class IAcademicTermRepository
{
public:
    virtual ~IAcademicTermRepository() = default;

    [[nodiscard]] virtual bool save(const AcademicTerm &term) = 0;
    [[nodiscard]] virtual std::optional<AcademicTerm> findById(
        const QUuid &id) const = 0;
    [[nodiscard]] virtual std::optional<std::vector<AcademicTerm>> findAll()
        const = 0;
};

} // namespace stukit::schedule
