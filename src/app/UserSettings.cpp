#include "UserSettings.h"

#include <QSettings>

namespace stukit::app {
namespace {

constexpr auto AccountingTimeZoneKey = "accounting/timeZoneId";

} // namespace

UserSettings::UserSettings(QSettings &settings)
    : m_settings(settings)
{
}

QTimeZone UserSettings::accountingTimeZone()
{
    const QByteArray id = m_settings.value(
        QString::fromLatin1(AccountingTimeZoneKey)).toByteArray();
    const QTimeZone storedTimeZone(id);
    if (storedTimeZone.isValid()) {
        return storedTimeZone;
    }

    const QTimeZone fallback = defaultAccountingTimeZone();
    setAccountingTimeZone(fallback);
    return fallback;
}

void UserSettings::setAccountingTimeZone(const QTimeZone &timeZone)
{
    if (!timeZone.isValid()) {
        return;
    }

    m_settings.setValue(QString::fromLatin1(AccountingTimeZoneKey),
                        timeZone.id());
}

QTimeZone UserSettings::defaultAccountingTimeZone()
{
    return QTimeZone::fromSecondsAheadOfUtc(8 * 60 * 60);
}

} // namespace stukit::app
