#pragma once

#include "IPlanFileWriter.h"

namespace stukit::planning {

class MarkdownPlanFileWriter final : public IPlanFileWriter
{
public:
    bool writeMarkdown(const QString &filePath,
                       const QString &markdownContent,
                       QString &error) override;
};

} // namespace stukit::planning
