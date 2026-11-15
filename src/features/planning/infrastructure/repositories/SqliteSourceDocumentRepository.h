#pragma once

#include "ISourceDocumentRepository.h"

#include <QSqlDatabase>

namespace stukit::planning {

class SqliteSourceDocumentRepository final
    : public ISourceDocumentRepository
{
public:
    explicit SqliteSourceDocumentRepository(QSqlDatabase database);

    bool save(const SourceDocument &document, QString &error) override;
    std::vector<SourceDocument> forProject(
        const QUuid &projectId, QString &error) const override;
    std::optional<SourceDocument> findById(
        const QUuid &id, QString &error) const override;
    bool remove(const QUuid &id, QString &error) override;

private:
    std::optional<SourceDocument> documentFromCurrentRow(
        const class QSqlQuery &query, QString &error) const;

    QSqlDatabase m_database;
};

} // namespace stukit::planning
