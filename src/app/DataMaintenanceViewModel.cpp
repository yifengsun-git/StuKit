#include "DataMaintenanceViewModel.h"

#include "AccountingModule.h"

#include <QFileInfo>

#include <utility>

namespace stukit::app {

DataMaintenanceViewModel::DataMaintenanceViewModel(
    AccountingModule &module,
    QObject *parent)
    : QObject(parent)
    , m_module(module)
{
}

bool DataMaintenanceViewModel::busy() const noexcept
{
    return m_busy;
}

bool DataMaintenanceViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &DataMaintenanceViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

const QString &DataMaintenanceViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
}

QString DataMaintenanceViewModel::lastSafetyBackupPath() const
{
    return m_module.lastSafetyBackupPath();
}

bool DataMaintenanceViewModel::backupTo(const QUrl &destination)
{
    beginOperation();
    const QString path = localPath(destination);
    if (path.isEmpty()) {
        finishWithError(QStringLiteral("请选择本机上的备份文件位置"));
        return false;
    }

    if (!m_module.backupDatabase(path, true)) {
        finishWithError(
            QStringLiteral("备份失败：%1").arg(m_module.lastError()));
        return false;
    }

    finishWithStatus(
        QStringLiteral("备份已保存到：%1")
            .arg(QFileInfo(path).absoluteFilePath()));
    return true;
}

bool DataMaintenanceViewModel::restoreFrom(const QUrl &source)
{
    beginOperation();
    const QString path = localPath(source);
    if (path.isEmpty()) {
        finishWithError(QStringLiteral("请选择本机上的 StuKit 备份文件"));
        return false;
    }

    const bool restored = m_module.restoreDatabase(path);
    emit accountingObjectsChanged();
    if (!restored) {
        finishWithError(
            QStringLiteral("恢复失败：%1").arg(m_module.lastError()));
        return false;
    }

    finishWithStatus(QStringLiteral(
        "数据恢复成功。恢复前的数据已自动保存为安全备份。"));
    return true;
}

void DataMaintenanceViewModel::clearMessages()
{
    if (m_errorMessage.isEmpty() && m_statusMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    m_statusMessage.clear();
    emit stateChanged();
}

QString DataMaintenanceViewModel::localPath(const QUrl &url)
{
    if (url.isLocalFile()) {
        return url.toLocalFile().trimmed();
    }
    if (url.scheme().isEmpty()) {
        return url.toString().trimmed();
    }
    return {};
}

void DataMaintenanceViewModel::beginOperation()
{
    m_busy = true;
    m_errorMessage.clear();
    m_statusMessage.clear();
    emit stateChanged();
}

void DataMaintenanceViewModel::finishWithError(QString message)
{
    m_busy = false;
    m_errorMessage = std::move(message);
    emit stateChanged();
}

void DataMaintenanceViewModel::finishWithStatus(QString message)
{
    m_busy = false;
    m_statusMessage = std::move(message);
    emit stateChanged();
}

} // namespace stukit::app
