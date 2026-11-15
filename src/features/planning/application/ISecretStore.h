#pragma once

#include <QString>

namespace stukit::planning {

class ISecretStore
{
public:
    virtual ~ISecretStore() = default;

    virtual bool writeSecret(const QString &name,
                             const QString &secret,
                             QString &error) = 0;
    virtual bool readSecret(const QString &name,
                            QString &secret,
                            QString &error) const = 0;
    virtual bool removeSecret(const QString &name, QString &error) = 0;
};

} // namespace stukit::planning
