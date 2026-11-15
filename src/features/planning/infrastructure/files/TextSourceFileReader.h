#pragma once

#include "ISourceFileReader.h"

namespace stukit::planning {

class TextSourceFileReader final : public ISourceFileReader
{
public:
    std::optional<SourceFileContent> read(
        const QString &filePath, QString &error) const override;
};

} // namespace stukit::planning
