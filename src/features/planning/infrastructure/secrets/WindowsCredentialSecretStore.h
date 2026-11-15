#pragma once

#include "ISecretStore.h"

namespace stukit::planning {

class WindowsCredentialSecretStore final : public ISecretStore
{
public:
    bool writeSecret(const QString &name,
                     const QString &secret,
                     QString &error) override;
    bool readSecret(const QString &name,
                    QString &secret,
                    QString &error) const override;
    bool removeSecret(const QString &name, QString &error) override;

private:
    static QString targetName(const QString &name);
};

} // namespace stukit::planning
