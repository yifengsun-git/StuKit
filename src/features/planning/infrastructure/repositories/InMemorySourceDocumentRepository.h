#pragma once

#include "ISourceDocumentRepository.h"

#include <vector>

namespace stukit::planning {

class InMemorySourceDocumentRepository final
    : public ISourceDocumentRepository
{
public:
    bool save(const SourceDocument &document, QString &error) override;
    std::vector<SourceDocument> forProject(
        const QUuid &projectId, QString &error) const override;
    std::optional<SourceDocument> findById(
        const QUuid &id, QString &error) const override;
    bool remove(const QUuid &id, QString &error) override;

private:
    std::vector<SourceDocument> m_documents;
};

} // namespace stukit::planning
