#pragma once

#include "ILanguageModelService.h"

#include <optional>

namespace stukit::planning {

class RoutingLanguageModelService final : public ILanguageModelService
{
public:
    RoutingLanguageModelService(
        ILanguageModelService &openAiCompatible,
        ILanguageModelService &anthropicMessages);

    void generate(LanguageModelRequest request,
                  Completion completion) override;
    void cancel() override;

private:
    ILanguageModelService &m_openAiCompatible;
    ILanguageModelService &m_anthropicMessages;
    std::optional<LanguageModelProtocol> m_activeProtocol;
};

} // namespace stukit::planning
