#pragma once

#include <QString>

#include <functional>

namespace stukit::planning {

enum class LanguageModelProtocol
{
    OpenAiCompatible,
    AnthropicMessages,
};

struct LanguageModelRequest
{
    LanguageModelProtocol protocol{LanguageModelProtocol::OpenAiCompatible};
    QString baseUrl;
    QString apiKey;
    QString modelId;
    QString workspaceId;
    QString systemPrompt;
    QString userPrompt;
    int maximumOutputTokens{4096};
};

struct LanguageModelResult
{
    bool succeeded{false};
    QString markdownContent;
    QString errorMessage;
    int promptTokens{0};
    int completionTokens{0};
};

class ILanguageModelService
{
public:
    using Completion = std::function<void(LanguageModelResult)>;

    virtual ~ILanguageModelService() = default;
    virtual void generate(LanguageModelRequest request,
                          Completion completion) = 0;
    virtual void cancel() = 0;
};

} // namespace stukit::planning
