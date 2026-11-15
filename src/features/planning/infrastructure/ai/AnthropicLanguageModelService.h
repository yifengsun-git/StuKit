#pragma once

#include "ILanguageModelService.h"

#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QPointer>

class QNetworkReply;

namespace stukit::planning {

class AnthropicLanguageModelService final : public ILanguageModelService
{
public:
    explicit AnthropicLanguageModelService(QNetworkAccessManager &network);
    ~AnthropicLanguageModelService() override;

    void generate(LanguageModelRequest request,
                  Completion completion) override;
    void cancel() override;

private:
    static QUrl messagesUrl(const QString &baseUrl);
    static QString networkErrorMessage(QNetworkReply &reply);

    QNetworkAccessManager &m_network;
    QPointer<QNetworkReply> m_activeReply;
    QMetaObject::Connection m_finishedConnection;
};

} // namespace stukit::planning
