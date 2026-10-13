#pragma once

#include <QString>
#include <QStringList>

#include <optional>

namespace stukit::tasks {

class ITaskCategoryStore
{
public:
    virtual ~ITaskCategoryStore() = default;

    [[nodiscard]] virtual std::optional<QStringList> categories() const = 0;
    [[nodiscard]] virtual bool addCategory(const QString &name) = 0;
};

} // namespace stukit::tasks
