#pragma once

#include "ICategoryRepository.h"

#include <algorithm>

namespace stukit::accounting::testing {

class InMemoryCategoryRepository final : public ICategoryRepository
{
public:
    [[nodiscard]] bool save(const Category &category) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_categories.begin(),
            m_categories.end(),
            [&category](const Category &stored) {
                return stored.id() == category.id();
            });

        if (existing == m_categories.end()) {
            m_categories.push_back(category);
        } else {
            *existing = category;
        }

        return true;
    }

    [[nodiscard]] std::optional<Category> findById(
        const QUuid &id) const override
    {
        const auto category = std::find_if(
            m_categories.cbegin(),
            m_categories.cend(),
            [&id](const Category &stored) {
                return stored.id() == id;
            });

        if (category == m_categories.cend()) {
            return std::nullopt;
        }

        return *category;
    }

    [[nodiscard]] std::optional<std::vector<Category>> findAll() const override
    {
        if (!m_findAllShouldSucceed) {
            return std::nullopt;
        }

        return m_categories;
    }

    void setSaveShouldSucceed(bool shouldSucceed) noexcept
    {
        m_saveShouldSucceed = shouldSucceed;
    }

    void setFindAllShouldSucceed(bool shouldSucceed) noexcept
    {
        m_findAllShouldSucceed = shouldSucceed;
    }

private:
    std::vector<Category> m_categories;
    bool m_saveShouldSucceed{true};
    bool m_findAllShouldSucceed{true};
};

} // namespace stukit::accounting::testing
