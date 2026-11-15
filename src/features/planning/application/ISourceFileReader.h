#pragma once

#include "PlanningTypes.h"

#include <QString>

#include <optional>

namespace stukit::planning {

struct SourceFileContent
{
    QString fileName;
    QString content;
    SourceDocumentFormat format{SourceDocumentFormat::PlainText};
};

class ISourceFileReader
{
public:
    virtual ~ISourceFileReader() = default;

    virtual std::optional<SourceFileContent> read(
        const QString &filePath, QString &error) const = 0;
};

} // namespace stukit::planning
