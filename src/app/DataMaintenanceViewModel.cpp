#include "DataMaintenanceViewModel.h"

#include "AccountingModule.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <memory>
#include <utility>

namespace stukit::app {

namespace {

bool isAndroidContentUrl(const QUrl &url)
{
    return url.scheme().compare(
        QStringLiteral("content"), Qt::CaseInsensitive) == 0;
}

QString fileNameForUrl(const QUrl &url)
{
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    if (url.scheme().isEmpty()) {
        return url.toString();
    }
    if (isAndroidContentUrl(url)) {
        return url.toString(QUrl::FullyEncoded);
    }
    return {};
}

QString stagingTemplate(const QString &baseName)
{
    QString directory = QStandardPaths::writableLocation(
        QStandardPaths::CacheLocation);
    if (directory.trimmed().isEmpty()) {
        directory = QDir::tempPath();
    }
    if (!QDir().mkpath(directory)) {
        return {};
    }
    return QDir(directory).filePath(baseName);
}

std::unique_ptr<QTemporaryFile> createStagingFile(
    const QString &baseName,
    QString *error)
{
    const QString fileTemplate = stagingTemplate(baseName);
    if (fileTemplate.isEmpty()) {
        *error = QStringLiteral("无法创建应用缓存目录");
        return nullptr;
    }

    auto file = std::make_unique<QTemporaryFile>(fileTemplate);
    if (!file->open()) {
        *error = file->errorString();
        return nullptr;
    }
    file->close();
    return file;
}

bool copyFile(const QString &sourceName, const QString &destinationName,
              QString *error)
{
    QFile source(sourceName);
    if (!source.open(QIODevice::ReadOnly)) {
        *error = source.errorString();
        return false;
    }

    QFile destination(destinationName);
    if (!destination.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = destination.errorString();
        return false;
    }

    while (!source.atEnd()) {
        const QByteArray chunk = source.read(64 * 1024);
        if (chunk.isEmpty() && source.error() != QFileDevice::NoError) {
            *error = source.errorString();
            return false;
        }
        if (destination.write(chunk) != chunk.size()) {
            *error = destination.errorString();
            return false;
        }
    }
    if (!destination.flush()) {
        *error = destination.errorString();
        return false;
    }
    return true;
}

} // namespace

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
    QString path = localPath(destination);
    std::unique_ptr<QTemporaryFile> stagingFile;
    QString transferError;
    if (path.isEmpty() && isAndroidContentUrl(destination)) {
        stagingFile = createStagingFile(
            QStringLiteral("stukit-backup-XXXXXX.sqlite"),
            &transferError);
        if (stagingFile != nullptr) {
            path = stagingFile->fileName();
        }
    }
    if (path.isEmpty()) {
        finishWithError(transferError.isEmpty()
            ? QStringLiteral("请选择备份文件位置")
            : QStringLiteral("无法准备备份文件：%1").arg(transferError));
        return false;
    }

    if (!m_module.backupDatabase(path, true)) {
        finishWithError(
            QStringLiteral("备份失败：%1").arg(m_module.lastError()));
        return false;
    }

    if (stagingFile != nullptr
        && !copyFile(
            path, fileNameForUrl(destination), &transferError)) {
        finishWithError(
            QStringLiteral("备份写入所选位置失败：%1")
                .arg(transferError));
        return false;
    }

    finishWithStatus(stagingFile != nullptr
        ? QStringLiteral("备份已保存到所选位置")
        : QStringLiteral("备份已保存到：%1")
            .arg(QFileInfo(path).absoluteFilePath()));
    return true;
}

bool DataMaintenanceViewModel::restoreFrom(const QUrl &source)
{
    beginOperation();
    QString path = localPath(source);
    std::unique_ptr<QTemporaryFile> stagingFile;
    QString transferError;
    if (path.isEmpty() && isAndroidContentUrl(source)) {
        stagingFile = createStagingFile(
            QStringLiteral("stukit-restore-XXXXXX.sqlite"),
            &transferError);
        if (stagingFile != nullptr
            && copyFile(
                fileNameForUrl(source),
                stagingFile->fileName(),
                &transferError)) {
            path = stagingFile->fileName();
        }
    }
    if (path.isEmpty()) {
        finishWithError(transferError.isEmpty()
            ? QStringLiteral("请选择 StuKit 备份文件")
            : QStringLiteral("无法读取所选备份：%1").arg(transferError));
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

bool DataMaintenanceViewModel::exportTransactionsTo(const QUrl &destination)
{
    beginOperation();
    QString path = localPath(destination);
    std::unique_ptr<QTemporaryFile> stagingFile;
    QString transferError;
    if (path.isEmpty() && isAndroidContentUrl(destination)) {
        stagingFile = createStagingFile(
            QStringLiteral("stukit-transactions-XXXXXX.csv"),
            &transferError);
        if (stagingFile != nullptr) {
            path = stagingFile->fileName();
        }
    }
    if (path.isEmpty()) {
        finishWithError(transferError.isEmpty()
            ? QStringLiteral("请选择 CSV 保存位置")
            : QStringLiteral("无法准备 CSV 文件：%1").arg(transferError));
        return false;
    }

    if (!m_module.exportTransactions(path, true)) {
        finishWithError(
            QStringLiteral("导出失败：%1").arg(m_module.lastError()));
        return false;
    }

    if (stagingFile != nullptr
        && !copyFile(
            path, fileNameForUrl(destination), &transferError)) {
        finishWithError(
            QStringLiteral("CSV 写入所选位置失败：%1")
                .arg(transferError));
        return false;
    }

    finishWithStatus(stagingFile != nullptr
        ? QStringLiteral("全部流水已导出到所选位置")
        : QStringLiteral("全部流水已导出到：%1")
            .arg(QFileInfo(path).absoluteFilePath()));
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
