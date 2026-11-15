#pragma once

#include "ILanguageModelService.h"

#include <QNetworkAccessManager>
#include <QPointer>
#include <QMetaObject>

class QNetworkReply;

namespace stukit::planning {

class OpenAiCompatibleLanguageModelService final
    : public ILanguageModelService
{
public:
    explicit OpenAiCompatibleLanguageModelService(
        QNetworkAccessManager &network);
    ~OpenAiCompatibleLanguageModelService() override;

    void generate(LanguageModelRequest request,
                  Completion completion) override;
    void cancel() override;

private:
    static QUrl chatCompletionsUrl(const QString &baseUrl);
    static QString networkErrorMessage(QNetworkReply &reply);

    QNetworkAccessManager &m_network;
    QPointer<QNetworkReply> m_activeReply;
    QMetaObject::Connection m_finishedConnection;
};

} // namespace stukit::planning
