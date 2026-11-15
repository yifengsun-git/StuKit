#include "InMemorySourceDocumentRepository.h"

#include <algorithm>

namespace stukit::planning {

bool InMemorySourceDocumentRepository::save(
    const SourceDocument &document, QString &error)
{
    error.clear();
    const auto duplicate = std::find_if(
        m_documents.cbegin(), m_documents.cend(),
        [&document](const SourceDocument &candidate) {
            return candidate.id() != document.id()
                && candidate.projectId() == document.projectId()
                && candidate.contentSha256() == document.contentSha256();
        });
    if (duplicate != m_documents.cend()) {
        error = QStringLiteral("该规划中已经存在内容相同的资料");
        return false;
    }
    const auto existing = std::find_if(
        m_documents.begin(), m_documents.end(),
        [&document](const SourceDocument &candidate) {
            return candidate.id() == document.id();
        });
    if (existing == m_documents.end()) {
        m_documents.push_back(document);
    } else {
        *existing = document;
    }
    return true;
}

std::optional<SourceDocument>
InMemorySourceDocumentRepository::findById(
    const QUuid &id, QString &error) const
{
    error.clear();
    const auto existing = std::find_if(
        m_documents.cbegin(), m_documents.cend(),
        [&id](const SourceDocument &document) { return document.id() == id; });
    if (existing == m_documents.cend()) {
        return std::nullopt;
    }
    return *existing;
}

bool InMemorySourceDocumentRepository::remove(
    const QUuid &id, QString &error)
{
    error.clear();
    const auto existing = std::find_if(
        m_documents.begin(), m_documents.end(),
        [&id](const SourceDocument &document) { return document.id() == id; });
    if (existing == m_documents.end()) {
        error = QStringLiteral("要删除的资料不存在");
        return false;
    }
    m_documents.erase(existing);
    return true;
}

std::vector<SourceDocument>
InMemorySourceDocumentRepository::forProject(
    const QUuid &projectId, QString &error) const
{
    error.clear();
    std::vector<SourceDocument> result;
    for (const auto &document : m_documents) {
        if (document.projectId() == projectId) {
            result.push_back(document);
        }
    }
    std::sort(
        result.begin(), result.end(),
        [](const SourceDocument &left, const SourceDocument &right) {
            return left.importedAtUtc() > right.importedAtUtc();
        });
    return result;
}

} // namespace stukit::planning
