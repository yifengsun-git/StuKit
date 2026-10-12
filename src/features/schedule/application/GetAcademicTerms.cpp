#include "GetAcademicTerms.h"

#include <algorithm>
#include <utility>

namespace stukit::schedule {

GetAcademicTerms::GetAcademicTerms(
    IAcademicTermRepository &termRepository)
    : m_termRepository(termRepository)
{
}

GetAcademicTermsResult GetAcademicTerms::execute() const
{
    auto terms = m_termRepository.findAll();
    if (!terms.has_value()) {
        return GetAcademicTermsError::StorageFailure;
    }

    std::ranges::sort(
        *terms,
        [](const AcademicTerm &left, const AcademicTerm &right) {
            if (left.isArchived() != right.isArchived()) {
                return !left.isArchived();
            }
            if (left.startDate() != right.startDate()) {
                return left.startDate() > right.startDate();
            }
            const int nameComparison = QString::compare(
                left.name(), right.name(), Qt::CaseInsensitive);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }
            return left.id().toString() < right.id().toString();
        });
    return std::move(*terms);
}

} // namespace stukit::schedule
