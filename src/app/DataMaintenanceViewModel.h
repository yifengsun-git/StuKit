#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

namespace stukit::app {

class AccountingModule;

class DataMaintenanceViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY stateChanged)
    Q_PROPERTY(QString lastSafetyBackupPath READ lastSafetyBackupPath NOTIFY stateChanged)

public:
    explicit DataMaintenanceViewModel(
        AccountingModule &module,
        QObject *parent = nullptr);

    [[nodiscard]] bool busy() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] const QString &statusMessage() const noexcept;
    [[nodiscard]] QString lastSafetyBackupPath() const;

    Q_INVOKABLE bool backupTo(const QUrl &destination);
    Q_INVOKABLE bool restoreFrom(const QUrl &source);
    Q_INVOKABLE bool exportTransactionsTo(const QUrl &destination);
    Q_INVOKABLE void clearMessages();

signals:
    void stateChanged();
    void accountingObjectsChanged();

private:
    [[nodiscard]] static QString localPath(const QUrl &url);
    void beginOperation();
    void finishWithError(QString message);
    void finishWithStatus(QString message);

    AccountingModule &m_module;
    bool m_busy{false};
    QString m_errorMessage;
    QString m_statusMessage;
};

} // namespace stukit::app
