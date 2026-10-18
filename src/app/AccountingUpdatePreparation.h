#pragma once

#include "IUpdatePreparation.h"

#include <QString>

namespace stukit::app {

class AccountingModule;

class AccountingUpdatePreparation final
    : public update::IUpdatePreparation
{
public:
    AccountingUpdatePreparation(
        AccountingModule &accountingModule,
        QString appDataDirectory);

    [[nodiscard]] bool prepare(
        const QString &targetVersion,
        QString &backupPath,
        QString &errorMessage) override;

private:
    AccountingModule &m_accountingModule;
    QString m_appDataDirectory;
};

} // namespace stukit::app
