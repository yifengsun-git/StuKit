#include "PlanningAnalysis.h"

#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <cmath>
#include <utility>

namespace stukit::planning {
namespace {

constexpr qsizetype MaximumFacts = 250;

QString shortExcerpt(const QString &line)
{
    const QString simplified = line.simplified();
    return simplified.size() > 220
        ? simplified.left(217) + QStringLiteral("…") : simplified;
}

bool isDeadlineLine(const QString &line)
{
    static const QRegularExpression expression(
        QStringLiteral("截止|截至|报名|申请|提交|考试"));
    return expression.match(line).hasMatch();
}

void appendIssue(PlanningFactExtractionResult &result,
                 PlanningValidationSeverity severity,
                 QString message,
                 std::optional<EvidenceReference> evidence = std::nullopt)
{
    result.issues.push_back(PlanningValidationIssue{
        .severity = severity,
        .message = std::move(message),
        .evidence = std::move(evidence),
    });
}

void appendFact(PlanningFactExtractionResult &result,
                QSet<QString> &seen,
                PlanningFactKind kind,
                QString label,
                QString rawValue,
                QString normalizedValue,
                const EvidenceReference &evidence)
{
    if (result.facts.size() >= MaximumFacts) {
        return;
    }
    const QString key = QStringLiteral("%1|%2|%3")
        .arg(static_cast<int>(kind))
        .arg(normalizedValue, evidence.citation());
    if (seen.contains(key)) {
        return;
    }
    seen.insert(key);
    result.facts.push_back(ExtractedPlanningFact{
        .kind = kind,
        .label = std::move(label),
        .rawValue = std::move(rawValue),
        .normalizedValue = std::move(normalizedValue),
        .evidence = evidence,
    });
}

} // namespace

QString EvidenceReference::citation() const
{
    return QStringLiteral("S%1:L%2").arg(sourceNumber).arg(lineNumber);
}

PlanningFactExtractionResult PlanningFactExtractor::extract(
    const std::vector<SourceDocument> &sources, const QDate &today)
{
    PlanningFactExtractionResult result;
    QSet<QString> seen;
    QSet<QString> totalCredits;

    static const QRegularExpression requirementExpression(
        QStringLiteral("应当|必须|需要|不得|至少|不少于|不低于|不超过|要求"));
    static const QRegularExpression creditExpression(
        QStringLiteral("(\\d+(?:\\.\\d+)?)\\s*学分"));
    static const QRegularExpression workloadExpression(
        QStringLiteral("(\\d+(?:\\.\\d+)?)\\s*(小时|学时|周)"));
    static const QRegularExpression moneyExpression(
        QStringLiteral("(?:￥|¥)?\\s*(\\d+(?:\\.\\d+)?)\\s*(万元|元)"));
    static const QRegularExpression fullDateExpression(
        QStringLiteral(
            "((?:19|20)\\d{2})[年./-](1[0-2]|0?[1-9])[月./-]"
            "(3[01]|[12]\\d|0?[1-9])日?"));
    static const QRegularExpression partialDateExpression(
        QStringLiteral("(?<!\\d)(1[0-2]|0?[1-9])月"
                       "(3[01]|[12]\\d|0?[1-9])日"));
    static const QRegularExpression totalCreditExpression(
        QStringLiteral("总学分|毕业[^。；，,]{0,16}学分"));

    for (qsizetype sourceIndex = 0;
         sourceIndex < static_cast<qsizetype>(sources.size());
         ++sourceIndex) {
        const SourceDocument &source = sources[
            static_cast<std::size_t>(sourceIndex)];
        const QStringList lines = source.content().split(QLatin1Char('\n'));
        for (qsizetype lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
            const QString line = lines.at(lineIndex).simplified();
            if (line.isEmpty()) {
                continue;
            }
            const EvidenceReference evidence{
                .sourceId = source.id(),
                .sourceNumber = static_cast<int>(sourceIndex + 1),
                .lineNumber = static_cast<int>(lineIndex + 1),
                .sourceTitle = source.title(),
                .excerpt = shortExcerpt(line),
            };

            if (requirementExpression.match(line).hasMatch()) {
                appendFact(result, seen, PlanningFactKind::Requirement,
                           QStringLiteral("约束要求"), line, line, evidence);
            }

            auto creditMatches = creditExpression.globalMatch(line);
            while (creditMatches.hasNext()) {
                const QRegularExpressionMatch match = creditMatches.next();
                const QString number = match.captured(1);
                appendFact(result, seen, PlanningFactKind::Credit,
                           QStringLiteral("学分"), match.captured(0),
                           number + QStringLiteral(" 学分"), evidence);
                if (totalCreditExpression.match(line).hasMatch()) {
                    totalCredits.insert(number);
                }
            }

            auto workloadMatches = workloadExpression.globalMatch(line);
            while (workloadMatches.hasNext()) {
                const QRegularExpressionMatch match = workloadMatches.next();
                appendFact(result, seen, PlanningFactKind::Workload,
                           QStringLiteral("时长 / 工作量"), match.captured(0),
                           match.captured(1) + QLatin1Char(' ')
                               + match.captured(2), evidence);
            }

            auto moneyMatches = moneyExpression.globalMatch(line);
            while (moneyMatches.hasNext()) {
                const QRegularExpressionMatch match = moneyMatches.next();
                const double multiplier = match.captured(2)
                    == QStringLiteral("万元") ? 10000.0 : 1.0;
                const double amount = match.captured(1).toDouble() * multiplier;
                appendFact(result, seen, PlanningFactKind::Money,
                           QStringLiteral("金额 / 预算"), match.captured(0),
                           QString::number(amount, 'f', 2)
                               + QStringLiteral(" 元"), evidence);
            }

            bool hasFullDate = false;
            auto fullDateMatches = fullDateExpression.globalMatch(line);
            while (fullDateMatches.hasNext()) {
                hasFullDate = true;
                const QRegularExpressionMatch match = fullDateMatches.next();
                const QDate date(match.captured(1).toInt(),
                                 match.captured(2).toInt(),
                                 match.captured(3).toInt());
                if (!date.isValid()) {
                    appendIssue(result, PlanningValidationSeverity::Error,
                        QStringLiteral("发现无效日期：%1").arg(match.captured(0)),
                        evidence);
                    continue;
                }
                const bool deadline = isDeadlineLine(line);
                appendFact(result, seen,
                           deadline ? PlanningFactKind::Deadline
                                    : PlanningFactKind::Date,
                           deadline ? QStringLiteral("截止日期")
                                    : QStringLiteral("日期"),
                           match.captured(0), date.toString(Qt::ISODate),
                           evidence);
                if (deadline && today.isValid() && date < today) {
                    appendIssue(result, PlanningValidationSeverity::Warning,
                        QStringLiteral("截止日期 %1 已早于今天")
                            .arg(date.toString(Qt::ISODate)), evidence);
                }
            }

            if (!hasFullDate) {
                auto partialDateMatches = partialDateExpression.globalMatch(line);
                while (partialDateMatches.hasNext()) {
                    const QRegularExpressionMatch match = partialDateMatches.next();
                    const QDate exampleDate(2000,
                        match.captured(1).toInt(), match.captured(2).toInt());
                    if (!exampleDate.isValid()) {
                        appendIssue(result, PlanningValidationSeverity::Error,
                            QStringLiteral("发现无效日期：%1")
                                .arg(match.captured(0)), evidence);
                        continue;
                    }
                    const bool deadline = isDeadlineLine(line);
                    const QString normalized = QStringLiteral("%1-%2（年份未指定）")
                        .arg(match.captured(1).toInt(), 2, 10, QLatin1Char('0'))
                        .arg(match.captured(2).toInt(), 2, 10, QLatin1Char('0'));
                    appendFact(result, seen,
                               deadline ? PlanningFactKind::Deadline
                                        : PlanningFactKind::Date,
                               deadline ? QStringLiteral("截止日期")
                                        : QStringLiteral("日期"),
                               match.captured(0), normalized, evidence);
                    appendIssue(result, PlanningValidationSeverity::Warning,
                        QStringLiteral("日期 %1 缺少年份，需要人工确认")
                            .arg(match.captured(0)), evidence);
                }
            }
        }
    }

    if (totalCredits.size() > 1) {
        QStringList values = totalCredits.values();
        values.sort();
        appendIssue(result, PlanningValidationSeverity::Warning,
            QStringLiteral("资料中出现多个毕业总学分值：%1，请核对适用版本")
                .arg(values.join(QStringLiteral("、"))));
    }
    if (result.facts.size() >= MaximumFacts) {
        appendIssue(result, PlanningValidationSeverity::Information,
            QStringLiteral("事实数量超过 %1 条，仅展示前 %1 条")
                .arg(MaximumFacts));
    }
    return result;
}

PlanningMarkdownValidationResult PlanningMarkdownValidator::validate(
    const QString &markdownContent, int sourceCount)
{
    PlanningMarkdownValidationResult result;
    const QString content = markdownContent.trimmed();
    if (content.isEmpty()) {
        result.errors.push_back(QStringLiteral("模型未返回规划正文"));
    }
    const QStringList requiredSections{
        QStringLiteral("目标摘要"),
        QStringLiteral("未来四周行动"),
        QStringLiteral("风险与假设"),
        QStringLiteral("资料引用"),
    };
    for (const QString &section : requiredSections) {
        if (!content.contains(section, Qt::CaseInsensitive)) {
            result.errors.push_back(
                QStringLiteral("缺少“%1”部分").arg(section));
        }
    }

    static const QRegularExpression citationExpression(
        QStringLiteral("\\[S(\\d+)(?::L\\d+)?\\]"));
    bool foundCitation = false;
    auto matches = citationExpression.globalMatch(content);
    while (matches.hasNext()) {
        foundCitation = true;
        const int number = matches.next().captured(1).toInt();
        if (number < 1 || number > sourceCount) {
            result.errors.push_back(
                QStringLiteral("引用 [S%1] 没有对应的资料").arg(number));
        }
    }
    if (!foundCitation) {
        result.errors.push_back(QStringLiteral("没有找到任何 [S1] 形式的资料引用"));
    }
    result.errors.removeDuplicates();
    result.valid = result.errors.isEmpty();
    return result;
}

std::vector<PlanningOpportunityAssessment> PlanningOpportunityScorer::score(
    const std::vector<SourceDocument> &sources,
    const PlanningFactExtractionResult &extraction,
    const QString &projectObjective,
    const QString &projectConstraints,
    const QDate &today)
{
    std::vector<PlanningOpportunityAssessment> assessments;
    static const QStringList knownKeywords{
        QStringLiteral("实习"), QStringLiteral("软件"),
        QStringLiteral("开发"), QStringLiteral("编程"),
        QStringLiteral("算法"), QStringLiteral("人工智能"),
        QStringLiteral("数据"), QStringLiteral("数学"),
        QStringLiteral("英语"), QStringLiteral("科研"),
        QStringLiteral("创新"), QStringLiteral("竞赛"),
        QStringLiteral("项目"), QStringLiteral("设计"),
        QStringLiteral("考研"), QStringLiteral("保研"),
        QStringLiteral("就业"), QStringLiteral("作品集"),
    };
    QStringList objectiveKeywords;
    for (const QString &keyword : knownKeywords) {
        if (projectObjective.contains(keyword, Qt::CaseInsensitive)) {
            objectiveKeywords.push_back(keyword);
        }
    }
    static const QRegularExpression asciiKeywordExpression(
        QStringLiteral("[A-Za-z][A-Za-z0-9+#.-]{1,}"));
    auto asciiMatches = asciiKeywordExpression.globalMatch(projectObjective);
    while (asciiMatches.hasNext()) {
        objectiveKeywords.push_back(
            asciiMatches.next().captured(0).toLower());
    }
    objectiveKeywords.removeDuplicates();

    double budgetLimit = -1.0;
    static const QRegularExpression budgetExpression(
        QStringLiteral("(?:预算|费用)[^。；，,]{0,16}?"
                       "(\\d+(?:\\.\\d+)?)\\s*(元|万元)"));
    const QRegularExpressionMatch budgetMatch =
        budgetExpression.match(projectConstraints);
    if (budgetMatch.hasMatch()) {
        budgetLimit = budgetMatch.captured(1).toDouble()
            * (budgetMatch.captured(2) == QStringLiteral("万元")
                ? 10000.0 : 1.0);
    }
    double weeklyHourLimit = -1.0;
    static const QRegularExpression hourExpression(
        QStringLiteral("每周[^。；，,]{0,16}?"
                       "(\\d+(?:\\.\\d+)?)\\s*小时"));
    const QRegularExpressionMatch hourMatch =
        hourExpression.match(projectConstraints);
    if (hourMatch.hasMatch()) {
        weeklyHourLimit = hourMatch.captured(1).toDouble();
    }

    for (qsizetype sourceIndex = 0;
         sourceIndex < static_cast<qsizetype>(sources.size());
         ++sourceIndex) {
        const SourceDocument &source = sources[
            static_cast<std::size_t>(sourceIndex)];
        if (source.kind() != SourceDocumentKind::Opportunity) {
            continue;
        }
        PlanningOpportunityAssessment assessment{
            .sourceId = source.id(),
            .sourceNumber = static_cast<int>(sourceIndex + 1),
            .title = source.title(),
        };

        QStringList matchedKeywords;
        for (const QString &keyword : objectiveKeywords) {
            if (source.content().contains(keyword, Qt::CaseInsensitive)) {
                matchedKeywords.push_back(keyword);
            }
        }
        assessment.relevanceScore = objectiveKeywords.isEmpty()
            ? 20 : std::min(40,
                10 + static_cast<int>(matchedKeywords.size()) * 10);
        if (matchedKeywords.isEmpty() && !objectiveKeywords.isEmpty()) {
            assessment.warnings.push_back(
                QStringLiteral("未找到与项目目标直接重合的关键词"));
        } else if (!matchedKeywords.isEmpty()) {
            assessment.reasons.push_back(
                QStringLiteral("目标关键词：%1")
                    .arg(matchedKeywords.join(QStringLiteral("、"))));
        } else {
            assessment.reasons.push_back(
                QStringLiteral("项目目标较宽泛，相关度暂按中性分计算"));
        }

        std::optional<QDate> earliestDeadline;
        double maximumCost = -1.0;
        double maximumHours = -1.0;
        QSet<int> evidenceKinds;
        for (const ExtractedPlanningFact &fact : extraction.facts) {
            if (fact.evidence.sourceId != source.id()) {
                continue;
            }
            evidenceKinds.insert(static_cast<int>(fact.kind));
            if (fact.kind == PlanningFactKind::Deadline
                && fact.normalizedValue.size() >= 10) {
                const QDate date = QDate::fromString(
                    fact.normalizedValue.left(10), Qt::ISODate);
                if (date.isValid()
                    && (!earliestDeadline || date < *earliestDeadline)) {
                    earliestDeadline = date;
                }
            } else if (fact.kind == PlanningFactKind::Money) {
                maximumCost = std::max(maximumCost,
                    fact.normalizedValue.section(QLatin1Char(' '), 0, 0)
                        .toDouble());
            } else if (fact.kind == PlanningFactKind::Workload
                       && fact.normalizedValue.endsWith(
                           QStringLiteral(" 小时"))) {
                maximumHours = std::max(maximumHours,
                    fact.normalizedValue.section(QLatin1Char(' '), 0, 0)
                        .toDouble());
            }
        }

        if (!earliestDeadline) {
            assessment.urgencyScore = 10;
            assessment.warnings.push_back(
                QStringLiteral("没有可确认年份的截止日期"));
        } else {
            const qint64 days = today.daysTo(*earliestDeadline);
            if (days < 0) {
                assessment.urgencyScore = 0;
                assessment.warnings.push_back(
                    QStringLiteral("截止日期已过去"));
            } else if (days <= 7) {
                assessment.urgencyScore = 8;
                assessment.warnings.push_back(
                    QStringLiteral("仅剩 %1 天，准备时间紧张").arg(days));
            } else if (days <= 30) {
                assessment.urgencyScore = 25;
                assessment.reasons.push_back(
                    QStringLiteral("距截止日期 %1 天，应尽快决策").arg(days));
            } else if (days <= 90) {
                assessment.urgencyScore = 22;
                assessment.reasons.push_back(
                    QStringLiteral("距截止日期 %1 天，准备窗口合理").arg(days));
            } else {
                assessment.urgencyScore = 15;
                assessment.reasons.push_back(
                    QStringLiteral("距截止日期较远，可纳入中期计划"));
            }
        }

        assessment.feasibilityScore = 25;
        if (budgetLimit >= 0.0 && maximumCost >= 0.0) {
            if (maximumCost > budgetLimit) {
                assessment.feasibilityScore -= 15;
                assessment.warnings.push_back(
                    QStringLiteral("资料金额 %1 元超过预算 %2 元")
                        .arg(maximumCost, 0, 'f', 2)
                        .arg(budgetLimit, 0, 'f', 2));
            } else {
                assessment.reasons.push_back(
                    QStringLiteral("已知费用在预算范围内"));
            }
        }
        if (weeklyHourLimit >= 0.0 && maximumHours >= 0.0) {
            if (maximumHours > weeklyHourLimit) {
                assessment.feasibilityScore -= 10;
                assessment.warnings.push_back(
                    QStringLiteral("每周投入 %1 小时超过限制 %2 小时")
                        .arg(maximumHours, 0, 'f', 1)
                        .arg(weeklyHourLimit, 0, 'f', 1));
            } else {
                assessment.reasons.push_back(
                    QStringLiteral("已知每周投入在时间限制内"));
            }
        }
        if (budgetLimit < 0.0 && weeklyHourLimit < 0.0) {
            assessment.warnings.push_back(
                QStringLiteral("项目未填写预算或每周时间限制，可行性暂按满分计算"));
        }
        assessment.feasibilityScore = std::clamp(
            assessment.feasibilityScore, 0, 25);
        assessment.evidenceScore = std::min(10,
            static_cast<int>(evidenceKinds.size()) * 2);
        if (assessment.evidenceScore < 6) {
            assessment.warnings.push_back(
                QStringLiteral("资料中的日期、费用或要求信息不够完整"));
        } else {
            assessment.reasons.push_back(
                QStringLiteral("资料包含 %1 类可验证事实")
                    .arg(evidenceKinds.size()));
        }
        assessment.totalScore = assessment.relevanceScore
            + assessment.urgencyScore + assessment.feasibilityScore
            + assessment.evidenceScore;
        assessments.push_back(std::move(assessment));
    }
    std::ranges::sort(assessments,
        [](const PlanningOpportunityAssessment &left,
           const PlanningOpportunityAssessment &right) {
            if (left.totalScore != right.totalScore) {
                return left.totalScore > right.totalScore;
            }
            return left.title.localeAwareCompare(right.title) < 0;
        });
    return assessments;
}

std::vector<PlanningActionDraft> PlanningActionExtractor::extract(
    const QString &markdownContent,
    const QString &projectTitle)
{
    std::vector<PlanningActionDraft> drafts;
    static const QRegularExpression uncheckedItemExpression(
        QStringLiteral("^\\s*[-*+]\\s+\\[\\s\\]\\s+(.+?)\\s*$"));
    QSet<QString> seenTitles;
    const QStringList lines = markdownContent.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QRegularExpressionMatch match =
            uncheckedItemExpression.match(line);
        if (!match.hasMatch()) {
            continue;
        }
        QString title = match.captured(1).trimmed();
        title.remove(QRegularExpression(
            QStringLiteral("\\s*\\[S\\d+(?::L\\d+)?\\]\\s*$")));
        title = title.trimmed();
        if (title.isEmpty() || seenTitles.contains(title)) {
            continue;
        }
        if (title.size() > 120) {
            title = title.left(117).trimmed() + QStringLiteral("…");
        }
        seenTitles.insert(title);
        drafts.push_back(PlanningActionDraft{
            .title = title,
            .description = QStringLiteral("来自 StuKit 规划「%1」。"
                                          "创建后请在待办中补充截止时间、"
                                          "优先级和课程关联。")
                               .arg(projectTitle),
        });
        if (drafts.size() >= 50) {
            break;
        }
    }
    return drafts;
}

QString planningFactKindName(PlanningFactKind kind)
{
    switch (kind) {
    case PlanningFactKind::Requirement:
        return QStringLiteral("要求");
    case PlanningFactKind::Deadline:
        return QStringLiteral("截止日期");
    case PlanningFactKind::Date:
        return QStringLiteral("日期");
    case PlanningFactKind::Credit:
        return QStringLiteral("学分");
    case PlanningFactKind::Workload:
        return QStringLiteral("时长");
    case PlanningFactKind::Money:
        return QStringLiteral("金额");
    }
    return {};
}

QString planningValidationSeverityName(PlanningValidationSeverity severity)
{
    switch (severity) {
    case PlanningValidationSeverity::Information:
        return QStringLiteral("提示");
    case PlanningValidationSeverity::Warning:
        return QStringLiteral("需核对");
    case PlanningValidationSeverity::Error:
        return QStringLiteral("错误");
    }
    return {};
}

} // namespace stukit::planning
