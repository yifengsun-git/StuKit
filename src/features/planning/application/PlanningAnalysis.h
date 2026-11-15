#pragma once

#include "IPlanningActionPublisher.h"
#include "SourceDocument.h"

#include <QDate>
#include <QString>
#include <QStringList>
#include <QUuid>

#include <optional>
#include <vector>

namespace stukit::planning {

enum class PlanningFactKind
{
    Requirement,
    Deadline,
    Date,
    Credit,
    Workload,
    Money,
};

struct EvidenceReference
{
    QUuid sourceId;
    int sourceNumber{0};
    int lineNumber{0};
    QString sourceTitle;
    QString excerpt;

    [[nodiscard]] QString citation() const;
};

struct ExtractedPlanningFact
{
    PlanningFactKind kind{PlanningFactKind::Requirement};
    QString label;
    QString rawValue;
    QString normalizedValue;
    EvidenceReference evidence;
};

enum class PlanningValidationSeverity
{
    Information,
    Warning,
    Error,
};

struct PlanningValidationIssue
{
    PlanningValidationSeverity severity{PlanningValidationSeverity::Warning};
    QString message;
    std::optional<EvidenceReference> evidence;
};

struct PlanningFactExtractionResult
{
    std::vector<ExtractedPlanningFact> facts;
    std::vector<PlanningValidationIssue> issues;
};

class PlanningFactExtractor final
{
public:
    static PlanningFactExtractionResult extract(
        const std::vector<SourceDocument> &sources,
        const QDate &today = QDate::currentDate());
};

struct PlanningMarkdownValidationResult
{
    bool valid{false};
    QStringList errors;
};

struct PlanningOpportunityAssessment
{
    QUuid sourceId;
    int sourceNumber{0};
    QString title;
    int totalScore{0};
    int relevanceScore{0};
    int urgencyScore{0};
    int feasibilityScore{0};
    int evidenceScore{0};
    QStringList reasons;
    QStringList warnings;
};

class PlanningOpportunityScorer final
{
public:
    static std::vector<PlanningOpportunityAssessment> score(
        const std::vector<SourceDocument> &sources,
        const PlanningFactExtractionResult &extraction,
        const QString &projectObjective,
        const QString &projectConstraints,
        const QDate &today = QDate::currentDate());
};

class PlanningMarkdownValidator final
{
public:
    static PlanningMarkdownValidationResult validate(
        const QString &markdownContent,
        int sourceCount);
};

class PlanningActionExtractor final
{
public:
    static std::vector<PlanningActionDraft> extract(
        const QString &markdownContent,
        const QString &projectTitle);
};

[[nodiscard]] QString planningFactKindName(PlanningFactKind kind);
[[nodiscard]] QString planningValidationSeverityName(
    PlanningValidationSeverity severity);

} // namespace stukit::planning
