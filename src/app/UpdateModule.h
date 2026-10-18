#pragma once

#include "AccountingUpdatePreparation.h"
#include "GitHubReleaseUpdateService.h"
#include "UpdateViewModel.h"
#include "WindowsUpdateInstaller.h"

namespace stukit::app {

class UpdateModule final
{
public:
    UpdateModule(
        AccountingModule &accountingModule,
        QString appDataDirectory,
        QString currentVersion);

    [[nodiscard]] update::UpdateViewModel *viewModel() noexcept;

private:
    update::GitHubReleaseUpdateService m_service;
    update::WindowsUpdateInstaller m_installer;
    AccountingUpdatePreparation m_preparation;
    update::UpdateViewModel m_viewModel;
};

} // namespace stukit::app
