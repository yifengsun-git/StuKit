#include "UpdateVersion.h"

#include <QVersionNumber>

namespace stukit::update {

QString UpdateVersion::normalized(const QString &version)
{
    QString result = version.trimmed();
    if (result.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        result.remove(0, 1);
    }
    return result;
}

bool UpdateVersion::isNewer(
    const QString &candidate,
    const QString &current)
{
    const QVersionNumber candidateVersion = QVersionNumber::fromString(
        normalized(candidate));
    const QVersionNumber currentVersion = QVersionNumber::fromString(
        normalized(current));
    if (candidateVersion.isNull() || currentVersion.isNull()) {
        return false;
    }
    return QVersionNumber::compare(candidateVersion, currentVersion) > 0;
}

} // namespace stukit::update
