#include "WindowsUpdateInstaller.h"

#include <QFileInfo>
#include <QProcess>

namespace stukit::update {

bool WindowsUpdateInstaller::supported() const noexcept
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

bool WindowsUpdateInstaller::startInstaller(
    const QString &packagePath,
    QString &errorMessage)
{
#ifdef Q_OS_WIN
    const QFileInfo package(packagePath);
    if (!package.exists() || !package.isFile()
        || package.suffix().compare(
            QStringLiteral("exe"), Qt::CaseInsensitive) != 0) {
        errorMessage = QStringLiteral("下载的 Windows 安装包不存在或格式不正确");
        return false;
    }
    if (!QProcess::startDetached(package.absoluteFilePath(), {})) {
        errorMessage = QStringLiteral("无法启动 Windows 安装程序");
        return false;
    }
    return true;
#else
    Q_UNUSED(packagePath)
    errorMessage = QStringLiteral("当前平台暂不支持应用内安装更新");
    return false;
#endif
}

} // namespace stukit::update
