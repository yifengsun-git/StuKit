#pragma once

#include "IAcademicTermRepository.h"

#include <QSqlDatabase>
#include <QString>

class QSqlQuery;

namespace stukit::schedule {

class SqliteAcademicTermRepository final : public IAcademicTermRepository
{
public:
    explicit SqliteAcademicTermRepository(QSqlDatabase database);

    [[nodiscard]] bool save(const AcademicTerm &term) override;
    [[nodiscard]] bool remove(const QUuid &id) override;
    [[nodiscard]] std::optional<AcademicTerm> findById(
        const QUuid &id) const override;
    [[nodiscard]] std::optional<std::vector<AcademicTerm>> findAll()
        const override;

    [[nodiscard]] const QString &lastError() const noexcept;

private:
    [[nodiscard]] std::optional<AcademicTerm> termFromCurrentRow(
        const QSqlQuery &query) const;

    QSqlDatabase m_database;
    mutable QString m_lastError;
};

} // namespace stukit::schedule
