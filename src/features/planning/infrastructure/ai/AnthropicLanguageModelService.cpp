#include "AnthropicLanguageModelService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace stukit::planning {

AnthropicLanguageModelService::AnthropicLanguageModelService(
    QNetworkAccessManager &network)
    : m_network(network)
{
}

AnthropicLanguageModelService::~AnthropicLanguageModelService()
{
    cancel();
}

void AnthropicLanguageModelService::generate(
    LanguageModelRequest request,
    Completion completion)
{
    cancel();
    const QUrl endpoint = messagesUrl(request.baseUrl);
    if (!endpoint.isValid() || endpoint.scheme() != QStringLiteral("https")) {
        completion(LanguageModelResult{
            .errorMessage = QStringLiteral("API 地址必须是有效的 HTTPS 地址"),
        });
        return;
    }
    if (request.apiKey.trimmed().isEmpty()
        || request.modelId.trimmed().isEmpty()) {
        completion(LanguageModelResult{
            .errorMessage = QStringLiteral("请先配置 Claude 模型名称和 API Key"),
        });
        return;
    }

    QNetworkRequest networkRequest(endpoint);
    networkRequest.setHeader(
        QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    networkRequest.setRawHeader(
        QByteArrayLiteral("Authorization"),
        QByteArrayLiteral("Bearer ") + request.apiKey.toUtf8());
    networkRequest.setRawHeader(
        QByteArrayLiteral("anthropic-version"),
        QByteArrayLiteral("2026-10-21"));
    if (!request.workspaceId.trimmed().isEmpty()) {
        networkRequest.setRawHeader(
            QByteArrayLiteral("anthropic-workspace-id"),
            request.workspaceId.trimmed().toUtf8());
    }
    networkRequest.setTransferTimeout(120000);

    const QJsonObject body{
        {QStringLiteral("model"), request.modelId.trimmed()},
        {QStringLiteral("system"), request.systemPrompt},
        {QStringLiteral("messages"), QJsonArray{
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("user")},
                {QStringLiteral("content"), request.userPrompt},
            },
        }},
        {QStringLiteral("max_tokens"), request.maximumOutputTokens},
        {QStringLiteral("stream"), false},
    };

    QNetworkReply *reply = m_network.post(
        networkRequest, QJsonDocument(body).toJson(QJsonDocument::Compact));
    m_activeReply = reply;
    m_finishedConnection = QObject::connect(
        reply, &QNetworkReply::finished, reply,
        [this, reply, completion = std::move(completion)]() mutable {
            if (m_activeReply == reply) {
                m_activeReply.clear();
                m_finishedConnection = {};
            }
            const QByteArray payload = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                completion(LanguageModelResult{
                    .errorMessage = networkErrorMessage(*reply),
                });
                reply->deleteLater();
                return;
            }

            QJsonParseError parseError;
            const QJsonObject root = QJsonDocument::fromJson(
                payload, &parseError).object();
            QString content;
            for (const QJsonValue &blockValue
                 : root.value(QStringLiteral("content")).toArray()) {
                const QJsonObject block = blockValue.toObject();
                if (block.value(QStringLiteral("type")).toString()
                    == QStringLiteral("text")) {
                    content += block.value(QStringLiteral("text")).toString();
                }
            }
            content = content.trimmed();
            if (parseError.error != QJsonParseError::NoError
                || content.isEmpty()) {
                completion(LanguageModelResult{
                    .errorMessage = QStringLiteral(
                        "Claude 返回了无法识别的响应，请检查 API 地址与模型名称"),
                });
                reply->deleteLater();
                return;
            }
            const QJsonObject usage =
                root.value(QStringLiteral("usage")).toObject();
            completion(LanguageModelResult{
                .succeeded = true,
                .markdownContent = content,
                .promptTokens = usage.value(
                    QStringLiteral("input_tokens")).toInt(),
                .completionTokens = usage.value(
                    QStringLiteral("output_tokens")).toInt(),
            });
            reply->deleteLater();
        });
}

void AnthropicLanguageModelService::cancel()
{
    if (!m_activeReply) {
        return;
    }
    QNetworkReply *reply = m_activeReply.data();
    m_activeReply.clear();
    QObject::disconnect(m_finishedConnection);
    m_finishedConnection = {};
    reply->abort();
    reply->deleteLater();
}

QUrl AnthropicLanguageModelService::messagesUrl(const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    if (!normalized.endsWith(QStringLiteral("/v1/messages"))) {
        normalized += normalized.endsWith(QStringLiteral("/v1"))
            ? QStringLiteral("/messages")
            : QStringLiteral("/v1/messages");
    }
    return QUrl(normalized);
}

QString AnthropicLanguageModelService::networkErrorMessage(
    QNetworkReply &reply)
{
    const int status = reply.attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 401 || status == 403) {
        return QStringLiteral("Claude API Key 无效、工作区未指定或没有模型权限");
    }
    if (status == 404) {
        return QStringLiteral("Claude API 地址或模型不存在（HTTP 404）");
    }
    if (status == 429) {
        return QStringLiteral("Claude 请求过于频繁或账户额度不足（HTTP 429）");
    }
    if (status >= 500) {
        return QStringLiteral("Claude 服务暂时不可用（HTTP %1）").arg(status);
    }
    return status > 0
        ? QStringLiteral("Claude 请求失败（HTTP %1）：%2")
            .arg(status).arg(reply.errorString())
        : QStringLiteral("Claude 请求失败：%1").arg(reply.errorString());
}

} // namespace stukit::planning
