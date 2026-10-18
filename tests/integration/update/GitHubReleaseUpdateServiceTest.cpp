#include "GitHubReleaseUpdateService.h"

#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <optional>
#include <variant>

using namespace stukit::update;

namespace {

constexpr auto installerName =
    "StuKit-0.3.0-windows-x64-setup.exe";

bool writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly)
        && file.write(content) == content.size();
}

QUrl writeRelease(
    const QTemporaryDir &directory,
    const QString &packagePath,
    const QByteArray &digest,
    qint64 packageSize)
{
    const QJsonObject asset{
        {QStringLiteral("name"), QString::fromLatin1(installerName)},
        {QStringLiteral("browser_download_url"),
         QUrl::fromLocalFile(packagePath).toString()},
        {QStringLiteral("size"), packageSize},
        {QStringLiteral("digest"),
         QStringLiteral("sha256:%1").arg(QString::fromLatin1(digest))},
    };
    const QJsonObject release{
        {QStringLiteral("tag_name"), QStringLiteral("v0.3.0")},
        {QStringLiteral("body"), QStringLiteral("本地更新测试")},
        {QStringLiteral("html_url"),
         QStringLiteral("https://example.invalid/releases/v0.3.0")},
        {QStringLiteral("published_at"),
         QStringLiteral("2026-10-18T12:00:00Z")},
        {QStringLiteral("assets"), QJsonArray{asset}},
    };
    const QString path = directory.filePath(QStringLiteral("latest.json"));
    if (!writeFile(path, QJsonDocument(release).toJson(
                             QJsonDocument::Compact))) {
        return {};
    }
    return QUrl::fromLocalFile(path);
}

template<typename StartOperation>
bool waitForOperation(StartOperation startOperation)
{
    QEventLoop loop;
    bool completed = false;
    bool timedOut = false;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        loop.quit();
    });
    startOperation(loop, completed);
    if (!completed) {
        timer.start(3'000);
        loop.exec();
    }
    return !timedOut && completed;
}

} // namespace

class GitHubReleaseUpdateServiceTest final : public QObject
{
    Q_OBJECT

private slots:
    void checksReleaseAndDownloadsVerifiedPackage();
    void removesDownloadWhenDigestDoesNotMatch();
    void canRetryImmediatelyAfterCancellation();
};

void GitHubReleaseUpdateServiceTest::checksReleaseAndDownloadsVerifiedPackage()
{
    QTemporaryDir sourceDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-source-test-XXXXXX")));
    QTemporaryDir downloadDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-download-test-XXXXXX")));
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(downloadDirectory.isValid());

    const QByteArray package("fake Windows installer bytes");
    const QString packagePath = sourceDirectory.filePath(
        QString::fromLatin1(installerName));
    QVERIFY(writeFile(packagePath, package));
    const QByteArray digest = QCryptographicHash::hash(
        package, QCryptographicHash::Sha256).toHex();
    const QUrl releaseUrl = writeRelease(
        sourceDirectory, packagePath, digest, package.size());
    QVERIFY(releaseUrl.isValid());

    GitHubReleaseUpdateService service(releaseUrl);
    std::optional<UpdateCheckResult> checkResult;
    QVERIFY(waitForOperation([&](QEventLoop &loop, bool &completed) {
        service.checkLatest([&](UpdateCheckResult result) {
            checkResult = std::move(result);
            completed = true;
            loop.quit();
        });
    }));
    QVERIFY(checkResult.has_value());
    const bool hasUpdate = std::holds_alternative<UpdateInfo>(*checkResult);
    const QString checkError = hasUpdate
        ? QString{}
        : std::get<UpdateFailure>(*checkResult).detail;
    QVERIFY2(hasUpdate, qPrintable(checkError));
    const UpdateInfo update = std::get<UpdateInfo>(*checkResult);
    QCOMPARE(update.version, QStringLiteral("0.3.0"));
    QCOMPARE(update.sha256, digest);
    QVERIFY(update.hasWindowsPackage());

    std::optional<UpdateDownloadResult> downloadResult;
    qint64 lastReceived = 0;
    QVERIFY(waitForOperation([&](QEventLoop &loop, bool &completed) {
        service.download(
            update,
            downloadDirectory.path(),
            [&](qint64 received, qint64) { lastReceived = received; },
            [&](UpdateDownloadResult result) {
                downloadResult = std::move(result);
                completed = true;
                loop.quit();
            });
    }));
    QVERIFY(downloadResult.has_value());
    QVERIFY(std::holds_alternative<QString>(*downloadResult));
    QCOMPARE(lastReceived, package.size());

    QFile downloaded(std::get<QString>(*downloadResult));
    QVERIFY(downloaded.open(QIODevice::ReadOnly));
    QCOMPARE(downloaded.readAll(), package);
}

void GitHubReleaseUpdateServiceTest::removesDownloadWhenDigestDoesNotMatch()
{
    QTemporaryDir sourceDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-corrupt-source-test-XXXXXX")));
    QTemporaryDir downloadDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-hash-test-XXXXXX")));
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(downloadDirectory.isValid());

    const QByteArray package("corrupted installer bytes");
    const QString packagePath = sourceDirectory.filePath(
        QString::fromLatin1(installerName));
    QVERIFY(writeFile(packagePath, package));
    const UpdateInfo update{
        .version = QStringLiteral("0.3.0"),
        .tagName = QStringLiteral("v0.3.0"),
        .packageFileName = QString::fromLatin1(installerName),
        .packageUrl = QUrl::fromLocalFile(packagePath),
        .packageSize = package.size(),
        .sha256 = QByteArray(64, '0'),
    };

    GitHubReleaseUpdateService service;
    std::optional<UpdateDownloadResult> result;
    QVERIFY(waitForOperation([&](QEventLoop &loop, bool &completed) {
        service.download(
            update,
            downloadDirectory.path(),
            [](qint64, qint64) {},
            [&](UpdateDownloadResult value) {
                result = std::move(value);
                completed = true;
                loop.quit();
            });
    }));
    QVERIFY(result.has_value());
    QVERIFY(std::holds_alternative<UpdateFailure>(*result));
    const UpdateFailure failure = std::get<UpdateFailure>(*result);
    QVERIFY2(failure.error == UpdateServiceError::IntegrityCheckFailed,
             qPrintable(failure.detail));
    QVERIFY(!QFile::exists(
        downloadDirectory.filePath(update.packageFileName)));
}

void GitHubReleaseUpdateServiceTest::canRetryImmediatelyAfterCancellation()
{
    QTemporaryDir sourceDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-retry-source-test-XXXXXX")));
    QTemporaryDir firstDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-retry-first-test-XXXXXX")));
    QTemporaryDir secondDirectory(QDir::current().filePath(
        QStringLiteral("stukit-update-retry-second-test-XXXXXX")));
    QVERIFY(sourceDirectory.isValid());
    QVERIFY(firstDirectory.isValid());
    QVERIFY(secondDirectory.isValid());

    const QByteArray package(1024 * 1024, 'x');
    const QString packagePath = sourceDirectory.filePath(
        QString::fromLatin1(installerName));
    QVERIFY(writeFile(packagePath, package));
    const UpdateInfo update{
        .version = QStringLiteral("0.3.0"),
        .tagName = QStringLiteral("v0.3.0"),
        .packageFileName = QString::fromLatin1(installerName),
        .packageUrl = QUrl::fromLocalFile(packagePath),
        .packageSize = package.size(),
        .sha256 = QCryptographicHash::hash(
            package, QCryptographicHash::Sha256).toHex(),
    };

    GitHubReleaseUpdateService service;
    std::optional<UpdateDownloadResult> cancelledResult;
    std::optional<UpdateDownloadResult> retryResult;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    const auto finishIfComplete = [&]() {
        if (cancelledResult.has_value() && retryResult.has_value()) {
            loop.quit();
        }
    };

    service.download(
        update,
        firstDirectory.path(),
        [](qint64, qint64) {},
        [&](UpdateDownloadResult result) {
            cancelledResult = std::move(result);
            finishIfComplete();
        });
    service.cancel();
    service.download(
        update,
        secondDirectory.path(),
        [](qint64, qint64) {},
        [&](UpdateDownloadResult result) {
            retryResult = std::move(result);
            finishIfComplete();
        });
    timeout.start(3'000);
    loop.exec();

    QVERIFY(cancelledResult.has_value());
    QVERIFY(retryResult.has_value());
    QVERIFY(std::holds_alternative<UpdateFailure>(*cancelledResult));
    QCOMPARE(std::get<UpdateFailure>(*cancelledResult).error,
             UpdateServiceError::Cancelled);
    QVERIFY(std::holds_alternative<QString>(*retryResult));
    QFile downloaded(std::get<QString>(*retryResult));
    QVERIFY(downloaded.open(QIODevice::ReadOnly));
    QCOMPARE(downloaded.readAll(), package);
}

QTEST_GUILESS_MAIN(GitHubReleaseUpdateServiceTest)

#include "GitHubReleaseUpdateServiceTest.moc"
