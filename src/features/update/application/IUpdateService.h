#pragma once

#include "UpdateInfo.h"

#include <QString>

#include <functional>
#include <variant>

namespace stukit::update {

enum class UpdateServiceError
{
    NetworkUnavailable,
    InvalidResponse,
    StorageUnavailable,
    IntegrityCheckFailed,
    Cancelled
};

struct UpdateFailure
{
    UpdateServiceError error{UpdateServiceError::InvalidResponse};
    QString detail;
};

using UpdateCheckResult = std::variant<UpdateInfo, UpdateFailure>;
using UpdateDownloadResult = std::variant<QString, UpdateFailure>;
using UpdateCheckCallback = std::function<void(UpdateCheckResult)>;
using UpdateProgressCallback = std::function<void(qint64, qint64)>;
using UpdateDownloadCallback = std::function<void(UpdateDownloadResult)>;

class IUpdateService
{
public:
    virtual ~IUpdateService() = default;

    virtual void checkLatest(UpdateCheckCallback callback) = 0;
    virtual void download(
        const UpdateInfo &update,
        const QString &destinationDirectory,
        UpdateProgressCallback progress,
        UpdateDownloadCallback callback) = 0;
    virtual void cancel() = 0;
};

} // namespace stukit::update
