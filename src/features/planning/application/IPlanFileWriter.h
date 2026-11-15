#pragma once

#include <QString>

namespace stukit::planning {

class IPlanFileWriter
{
public:
    virtual ~IPlanFileWriter() = default;
    virtual bool writeMarkdown(const QString &filePath,
                               const QString &markdownContent,
                               QString &error) = 0;
};

} // namespace stukit::planning
