#include "RoutingLanguageModelService.h"

#include <utility>

namespace stukit::planning {

RoutingLanguageModelService::RoutingLanguageModelService(
    ILanguageModelService &openAiCompatible,
    ILanguageModelService &anthropicMessages)
    : m_openAiCompatible(openAiCompatible)
    , m_anthropicMessages(anthropicMessages)
{
}

void RoutingLanguageModelService::generate(
    LanguageModelRequest request,
    Completion completion)
{
    cancel();
    const LanguageModelProtocol protocol = request.protocol;
    m_activeProtocol = protocol;
    ILanguageModelService &service =
        protocol == LanguageModelProtocol::AnthropicMessages
        ? m_anthropicMessages : m_openAiCompatible;
    service.generate(std::move(request),
        [this, protocol, completion = std::move(completion)](
            LanguageModelResult result) mutable {
            if (m_activeProtocol == protocol) {
                m_activeProtocol.reset();
            }
            completion(std::move(result));
        });
}

void RoutingLanguageModelService::cancel()
{
    if (!m_activeProtocol) {
        return;
    }
    const LanguageModelProtocol protocol = *m_activeProtocol;
    m_activeProtocol.reset();
    if (protocol == LanguageModelProtocol::AnthropicMessages) {
        m_anthropicMessages.cancel();
    } else {
        m_openAiCompatible.cancel();
    }
}

} // namespace stukit::planning
