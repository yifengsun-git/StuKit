#pragma once

namespace stukit::planning {

enum class PlanningProjectStatus
{
    Draft,
    Active,
    Archived,
};

enum class SourceDocumentKind
{
    Curriculum,
    Opportunity,
    PersonalProfile,
    Other,
};

enum class SourceDocumentFormat
{
    PlainText,
    Markdown,
    Csv,
};

} // namespace stukit::planning
