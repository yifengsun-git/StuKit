#pragma once

#include "ICourseRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::schedule {

class SqliteCourseRepository final : public ICourseRepository
{
public:
    explicit SqliteCourseRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const Course &course) override;
    [[nodiscard]] std::optional<Course> findById(
        const QUuid &id) const override;
    [[nodiscard]] std::optional<std::vector<Course>> findByTermId(
        const QUuid &termId) const override;
    [[nodiscard]] std::optional<std::vector<Course>> findAll() const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<Course> courseFromCurrentRow(
        const QSqlQuery &query) const;
    [[nodiscard]] std::optional<std::vector<Course>> coursesFromQuery(
        QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::schedule
