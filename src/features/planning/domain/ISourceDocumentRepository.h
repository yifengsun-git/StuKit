#pragma once

#include "SourceDocument.h"

#include <QString>
#include <QUuid>

#include <vector>

namespace stukit::planning {

class ISourceDocumentRepository
{
public:
    virtual ~ISourceDocumentRepository() = default;

    virtual bool save(const SourceDocument &document, QString &error) = 0;
    virtual std::vector<SourceDocument> forProject(
        const QUuid &projectId, QString &error) const = 0;
    virtual std::optional<SourceDocument> findById(
        const QUuid &id, QString &error) const = 0;
    virtual bool remove(const QUuid &id, QString &error) = 0;
};

} // namespace stukit::planning
