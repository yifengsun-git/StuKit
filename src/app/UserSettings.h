#pragma once

#include <QTimeZone>

class QSettings;

namespace stukit::app {

class UserSettings final
{
public:
    explicit UserSettings(QSettings &settings);

    [[nodiscard]] QTimeZone accountingTimeZone();
    void setAccountingTimeZone(const QTimeZone &timeZone);

    [[nodiscard]] static QTimeZone defaultAccountingTimeZone();

private:
    QSettings &m_settings;
};

} // namespace stukit::app
