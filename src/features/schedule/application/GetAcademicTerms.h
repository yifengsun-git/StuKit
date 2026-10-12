#pragma once

#include "IAcademicTermRepository.h"

#include <variant>
#include <vector>

namespace stukit::schedule {

enum class GetAcademicTermsError
{
    StorageFailure
};

using GetAcademicTermsResult = std::variant<
    std::vector<AcademicTerm>, GetAcademicTermsError>;

class GetAcademicTerms final
{
public:
    explicit GetAcademicTerms(IAcademicTermRepository &termRepository);

    [[nodiscard]] GetAcademicTermsResult execute() const;

private:
    IAcademicTermRepository &m_termRepository;
};

} // namespace stukit::schedule
