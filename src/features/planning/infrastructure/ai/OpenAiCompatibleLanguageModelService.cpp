#include "OpenAiCompatibleLanguageModelService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace stukit::planning {

OpenAiCompatibleLanguageModelService::OpenAiCompatibleLanguageModelService(
    QNetworkAccessManager &network)
    : m_network(network)
{
}

OpenAiCompatibleLanguageModelService::~OpenAiCompatibleLanguageModelService()
{
    cancel();
}

void OpenAiCompatibleLanguageModelService::generate(
    LanguageModelRequest request,
    Completion completion)
{
    cancel();
    const QUrl endpoint = chatCompletionsUrl(request.baseUrl);
    if (!endpoint.isValid() || endpoint.scheme() != QStringLiteral("https")) {
        completion(LanguageModelResult{
            .errorMessage = QStringLiteral("API 地址必须是有效的 HTTPS 地址"),
        });
        return;
    }
    if (request.apiKey.trimmed().isEmpty()
        || request.modelId.trimmed().isEmpty()) {
        completion(LanguageModelResult{
            .errorMessage = QStringLiteral("请先配置模型名称和 API Key"),
        });
        return;
    }

    QNetworkRequest networkRequest(endpoint);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader,
                             QStringLiteral("application/json"));
    networkRequest.setRawHeader(
        QByteArrayLiteral("Authorization"),
        QByteArrayLiteral("Bearer ") + request.apiKey.toUtf8());
    networkRequest.setTransferTimeout(120000);

    const QJsonObject body{
        {QStringLiteral("model"), request.modelId.trimmed()},
        {QStringLiteral("messages"), QJsonArray{
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("system")},
                {QStringLiteral("content"), request.systemPrompt},
            },
            QJsonObject{
                {QStringLiteral("role"), QStringLiteral("user")},
                {QStringLiteral("content"), request.userPrompt},
            },
        }},
        {QStringLiteral("stream"), false},
        {QStringLiteral("temperature"), 0.3},
        {QStringLiteral("max_tokens"), request.maximumOutputTokens},
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
            const QJsonDocument document =
                QJsonDocument::fromJson(payload, &parseError);
            const QJsonObject root = document.object();
            const QJsonArray choices = root.value(
                QStringLiteral("choices")).toArray();
            const QString content = (choices.isEmpty()
                ? QJsonObject{} : choices.at(0).toObject())
                .value(QStringLiteral("message")).toObject()
                .value(QStringLiteral("content")).toString().trimmed();
            if (parseError.error != QJsonParseError::NoError
                || content.isEmpty()) {
                completion(LanguageModelResult{
                    .errorMessage = QStringLiteral(
                        "模型返回了无法识别的响应，请检查 API 地址与模型名称"),
                });
                reply->deleteLater();
                return;
            }
            const QJsonObject usage = root.value(
                QStringLiteral("usage")).toObject();
            completion(LanguageModelResult{
                .succeeded = true,
                .markdownContent = content,
                .promptTokens = usage.value(
                    QStringLiteral("prompt_tokens")).toInt(),
                .completionTokens = usage.value(
                    QStringLiteral("completion_tokens")).toInt(),
            });
            reply->deleteLater();
        });
}

void OpenAiCompatibleLanguageModelService::cancel()
{
    if (m_activeReply) {
        QNetworkReply *reply = m_activeReply.data();
        m_activeReply.clear();
        QObject::disconnect(m_finishedConnection);
        m_finishedConnection = {};
        reply->abort();
        reply->deleteLater();
    }
}

QUrl OpenAiCompatibleLanguageModelService::chatCompletionsUrl(
    const QString &baseUrl)
{
    QString normalized = baseUrl.trimmed();
    while (normalized.endsWith(QLatin1Char('/'))) {
        normalized.chop(1);
    }
    if (!normalized.endsWith(QStringLiteral("/chat/completions"))) {
        if (normalized.endsWith(QStringLiteral("/v1"))) {
            normalized += QStringLiteral("/chat/completions");
        } else {
            normalized += QStringLiteral("/chat/completions");
        }
    }
    return QUrl(normalized);
}

QString OpenAiCompatibleLanguageModelService::networkErrorMessage(
    QNetworkReply &reply)
{
    const int status = reply.attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 401 || status == 403) {
        return QStringLiteral("API Key 无效或没有模型访问权限");
    }
    if (status == 404) {
        return QStringLiteral("API 地址或模型不存在（HTTP 404）");
    }
    if (status == 429) {
        return QStringLiteral("请求过于频繁或账户额度不足（HTTP 429）");
    }
    if (status >= 500) {
        return QStringLiteral("模型服务暂时不可用（HTTP %1）").arg(status);
    }
    return status > 0
        ? QStringLiteral("模型请求失败（HTTP %1）：%2")
            .arg(status).arg(reply.errorString())
        : QStringLiteral("模型请求失败：%1").arg(reply.errorString());
}

} // namespace stukit::planning
