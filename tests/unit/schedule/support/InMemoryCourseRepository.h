#pragma once

#include "ICourseRepository.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace stukit::schedule::testing {

class InMemoryCourseRepository final : public ICourseRepository
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

    bool save(const Course &course) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_courses.begin(),
            m_courses.end(),
            [&course](const Course &candidate) {
                return candidate.id() == course.id();
            });
        if (existing == m_courses.end()) {
            m_courses.push_back(course);
        } else {
            *existing = course;
        }
        return true;
    }

    bool remove(const QUuid &id) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }
        const auto oldSize = m_courses.size();
        std::erase_if(
            m_courses,
            [&id](const Course &course) {
                return course.id() == id;
            });
        return m_courses.size() + 1 == oldSize;
    }

    std::optional<Course> findById(const QUuid &id) const override
    {
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }
        const auto course = std::find_if(
            m_courses.cbegin(),
            m_courses.cend(),
            [&id](const Course &candidate) {
                return candidate.id() == id;
            });
        return course == m_courses.cend()
            ? std::nullopt
            : std::optional<Course>(*course);
    }

    std::optional<std::vector<Course>> findByTermId(
        const QUuid &termId) const override
    {
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }

        std::vector<Course> matches;
        std::ranges::copy_if(
            m_courses,
            std::back_inserter(matches),
            [&termId](const Course &course) {
                return course.termId() == termId;
            });
        return matches;
    }

    std::optional<std::vector<Course>> findAll() const override
    {
        ++m_findAllCallCount;
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }
        return m_courses;
    }

    [[nodiscard]] int findAllCallCount() const noexcept
    {
        return m_findAllCallCount;
    }

private:
    bool m_saveShouldSucceed{true};
    bool m_queryShouldSucceed{true};
    mutable int m_findAllCallCount{0};
    std::vector<Course> m_courses;
};

} // namespace stukit::schedule::testing
