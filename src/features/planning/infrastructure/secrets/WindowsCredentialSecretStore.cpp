#include "WindowsCredentialSecretStore.h"

#include <QByteArray>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <wincred.h>
#endif

namespace stukit::planning {

bool WindowsCredentialSecretStore::writeSecret(
    const QString &name, const QString &secret, QString &error)
{
    error.clear();
#ifdef Q_OS_WIN
    const QString target = targetName(name);
    const QByteArray bytes = secret.toUtf8();
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<wchar_t *>(
        reinterpret_cast<const wchar_t *>(target.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(bytes.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(
        const_cast<char *>(bytes.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<wchar_t *>(L"StuKit");
    if (!CredWriteW(&credential, 0)) {
        error = QStringLiteral("无法写入 Windows 凭据管理器（错误 %1）")
                    .arg(GetLastError());
        return false;
    }
    return true;
#else
    Q_UNUSED(name)
    Q_UNUSED(secret)
    error = QStringLiteral("当前平台暂不支持安全保存 API Key");
    return false;
#endif
}

bool WindowsCredentialSecretStore::readSecret(
    const QString &name, QString &secret, QString &error) const
{
    secret.clear();
    error.clear();
#ifdef Q_OS_WIN
    const QString target = targetName(name);
    PCREDENTIALW credential = nullptr;
    if (!CredReadW(reinterpret_cast<LPCWSTR>(target.utf16()),
                   CRED_TYPE_GENERIC, 0, &credential)) {
        const DWORD code = GetLastError();
        if (code == ERROR_NOT_FOUND) {
            return true;
        }
        error = QStringLiteral("无法读取 Windows 凭据管理器（错误 %1）")
                    .arg(code);
        return false;
    }
    secret = QString::fromUtf8(
        reinterpret_cast<const char *>(credential->CredentialBlob),
        static_cast<qsizetype>(credential->CredentialBlobSize));
    CredFree(credential);
    return true;
#else
    Q_UNUSED(name)
    error = QStringLiteral("当前平台暂不支持读取 API Key");
    return false;
#endif
}

bool WindowsCredentialSecretStore::removeSecret(
    const QString &name, QString &error)
{
    error.clear();
#ifdef Q_OS_WIN
    const QString target = targetName(name);
    if (!CredDeleteW(reinterpret_cast<LPCWSTR>(target.utf16()),
                     CRED_TYPE_GENERIC, 0)) {
        const DWORD code = GetLastError();
        if (code == ERROR_NOT_FOUND) {
            return true;
        }
        error = QStringLiteral("无法删除 Windows 凭据（错误 %1）").arg(code);
        return false;
    }
    return true;
#else
    Q_UNUSED(name)
    error = QStringLiteral("当前平台暂不支持删除 API Key");
    return false;
#endif
}

QString WindowsCredentialSecretStore::targetName(const QString &name)
{
    return QStringLiteral("StuKit/Planning/%1").arg(name);
}

} // namespace stukit::planning
