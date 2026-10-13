#pragma once

#include "ICourseMeetingRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::schedule {

class SqliteCourseMeetingRepository final : public ICourseMeetingRepository
{
public:
    explicit SqliteCourseMeetingRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const CourseMeeting &meeting) override;
    [[nodiscard]] bool remove(const QUuid &id) override;
    [[nodiscard]] bool removeByCourseId(const QUuid &courseId) override;
    [[nodiscard]] std::optional<CourseMeeting> findById(
        const QUuid &id) const override;
    [[nodiscard]] std::optional<std::vector<CourseMeeting>> findByCourseId(
        const QUuid &courseId) const override;
    [[nodiscard]] std::optional<std::vector<CourseMeeting>> findAll()
        const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<CourseMeeting> meetingFromCurrentRow(
        const QSqlQuery &query) const;
    [[nodiscard]] std::optional<std::vector<CourseMeeting>> meetingsFromQuery(
        QSqlQuery &query) const;
    [[nodiscard]] std::optional<std::vector<int>> activeWeeksFor(
        const QUuid &meetingId) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::schedule
