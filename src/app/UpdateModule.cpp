#include "UpdateModule.h"

#include <utility>

namespace stukit::app {

UpdateModule::UpdateModule(
    AccountingModule &accountingModule,
    QString appDataDirectory,
    QString currentVersion)
    : m_preparation(accountingModule, std::move(appDataDirectory))
    , m_viewModel(
        m_service,
        m_installer,
        m_preparation,
        std::move(currentVersion))
{
}

update::UpdateViewModel *UpdateModule::viewModel() noexcept
{
    return &m_viewModel;
}

} // namespace stukit::app
