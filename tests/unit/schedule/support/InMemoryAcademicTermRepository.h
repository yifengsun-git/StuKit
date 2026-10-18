#pragma once

#include "IAcademicTermRepository.h"

#include <algorithm>
#include <vector>

namespace stukit::schedule::testing {

class InMemoryAcademicTermRepository final : public IAcademicTermRepository
{
public:
    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

    void setQueryShouldSucceed(bool shouldSucceed) noexcept
    {
        m_queryShouldSucceed = shouldSucceed;
    }

    bool save(const AcademicTerm &term) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_terms.begin(),
            m_terms.end(),
            [&term](const AcademicTerm &candidate) {
                return candidate.id() == term.id();
            });
        if (existing == m_terms.end()) {
            m_terms.push_back(term);
        } else {
            *existing = term;
        }
        return true;
    }

    bool remove(const QUuid &id) override
    {
        const auto existing = std::find_if(
            m_terms.begin(), m_terms.end(),
            [&id](const AcademicTerm &candidate) {
                return candidate.id() == id;
            });
        if (existing == m_terms.end()) {
            return false;
        }
        m_terms.erase(existing);
        return true;
    }

    std::optional<AcademicTerm> findById(const QUuid &id) const override
    {
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }
        const auto term = std::find_if(
            m_terms.cbegin(),
            m_terms.cend(),
            [&id](const AcademicTerm &candidate) {
                return candidate.id() == id;
            });
        return term == m_terms.cend()
            ? std::nullopt
            : std::optional<AcademicTerm>(*term);
    }

    std::optional<std::vector<AcademicTerm>> findAll() const override
    {
        ++m_findAllCallCount;
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }
        return m_terms;
    }

    [[nodiscard]] int findAllCallCount() const noexcept
    {
        return m_findAllCallCount;
    }

private:
    bool m_saveShouldSucceed{true};
    bool m_queryShouldSucceed{true};
    mutable int m_findAllCallCount{0};
    std::vector<AcademicTerm> m_terms;
};

} // namespace stukit::schedule::testing
