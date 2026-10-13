#pragma once

#include "ICourseMeetingRepository.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace stukit::schedule::testing {

class InMemoryCourseMeetingRepository final : public ICourseMeetingRepository
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

    bool save(const CourseMeeting &meeting) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }

        const auto existing = std::find_if(
            m_meetings.begin(),
            m_meetings.end(),
            [&meeting](const CourseMeeting &candidate) {
                return candidate.id() == meeting.id();
            });
        if (existing == m_meetings.end()) {
            m_meetings.push_back(meeting);
        } else {
            *existing = meeting;
        }
        return true;
    }

    bool remove(const QUuid &id) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }
        const auto oldSize = m_meetings.size();
        std::erase_if(
            m_meetings,
            [&id](const CourseMeeting &meeting) {
                return meeting.id() == id;
            });
        return m_meetings.size() + 1 == oldSize;
    }

    bool removeByCourseId(const QUuid &courseId) override
    {
        if (!m_saveShouldSucceed) {
            return false;
        }
        std::erase_if(
            m_meetings,
            [&courseId](const CourseMeeting &meeting) {
                return meeting.courseId() == courseId;
            });
        return true;
    }

    std::optional<CourseMeeting> findById(const QUuid &id) const override
    {
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }
        const auto meeting = std::find_if(
            m_meetings.cbegin(),
            m_meetings.cend(),
            [&id](const CourseMeeting &candidate) {
                return candidate.id() == id;
            });
        return meeting == m_meetings.cend()
            ? std::nullopt
            : std::optional<CourseMeeting>(*meeting);
    }

    std::optional<std::vector<CourseMeeting>> findByCourseId(
        const QUuid &courseId) const override
    {
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }

        std::vector<CourseMeeting> matches;
        std::ranges::copy_if(
            m_meetings,
            std::back_inserter(matches),
            [&courseId](const CourseMeeting &meeting) {
                return meeting.courseId() == courseId;
            });
        return matches;
    }

    std::optional<std::vector<CourseMeeting>> findAll() const override
    {
        ++m_findAllCallCount;
        if (!m_queryShouldSucceed) {
            return std::nullopt;
        }
        return m_meetings;
    }

    [[nodiscard]] int findAllCallCount() const noexcept
    {
        return m_findAllCallCount;
    }

private:
    bool m_saveShouldSucceed{true};
    bool m_queryShouldSucceed{true};
    mutable int m_findAllCallCount{0};
    std::vector<CourseMeeting> m_meetings;
};

} // namespace stukit::schedule::testing
