#include "GitHubReleaseParser.h"
#include "UpdateVersion.h"

#include <QtTest>

using namespace stukit::update;

class UpdateTest final : public QObject
{
    Q_OBJECT

private slots:
    void normalizesVersionTags();
    void comparesSemanticVersions();
    void parsesWindowsRelease();
    void acceptsReleaseWithoutWindowsPackage();
    void rejectsInvalidReleaseJson();
    void rejectsInvalidVersionTag();
};

void UpdateTest::normalizesVersionTags()
{
    QCOMPARE(UpdateVersion::normalized(QStringLiteral(" v0.2.0 ")),
             QStringLiteral("0.2.0"));
    QCOMPARE(UpdateVersion::normalized(QStringLiteral("1.4.3")),
             QStringLiteral("1.4.3"));
}

void UpdateTest::comparesSemanticVersions()
{
    QVERIFY(UpdateVersion::isNewer(
        QStringLiteral("v0.2.0"), QStringLiteral("0.1.0")));
    QVERIFY(UpdateVersion::isNewer(
        QStringLiteral("1.0.1"), QStringLiteral("1.0.0")));
    QVERIFY(!UpdateVersion::isNewer(
        QStringLiteral("0.1.0"), QStringLiteral("0.1.0")));
    QVERIFY(!UpdateVersion::isNewer(
        QStringLiteral("invalid"), QStringLiteral("0.1.0")));
}

void UpdateTest::parsesWindowsRelease()
{
    const QByteArray json = R"json({
        "tag_name": "v0.2.0",
        "body": "新增 Windows 手动更新",
        "html_url": "https://github.com/yifengsun-git/StuKit/releases/tag/v0.2.0",
        "published_at": "2026-10-18T10:00:00Z",
        "assets": [
            {
                "name": "StuKit-windows-x64.zip",
                "browser_download_url": "https://example.invalid/portable.zip",
                "size": 12
            },
            {
                "name": "StuKit-0.2.0-windows-x64-setup.exe",
                "browser_download_url": "https://example.invalid/setup.exe",
                "size": 10485760,
                "digest": "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            }
        ]
    })json";

    const UpdateCheckResult result = GitHubReleaseParser::parse(json);
    QVERIFY(std::holds_alternative<UpdateInfo>(result));
    const UpdateInfo info = std::get<UpdateInfo>(result);
    QCOMPARE(info.version, QStringLiteral("0.2.0"));
    QCOMPARE(info.packageFileName,
             QStringLiteral("StuKit-0.2.0-windows-x64-setup.exe"));
    QCOMPARE(info.packageSize, 10LL * 1024 * 1024);
    QCOMPARE(info.sha256, QByteArray(64, 'a'));
    QVERIFY(info.hasWindowsPackage());
}

void UpdateTest::acceptsReleaseWithoutWindowsPackage()
{
    const QByteArray json = R"json({
        "tag_name": "v0.3.0",
        "html_url": "https://example.invalid/release",
        "assets": []
    })json";

    const UpdateCheckResult result = GitHubReleaseParser::parse(json);
    QVERIFY(std::holds_alternative<UpdateInfo>(result));
    QVERIFY(!std::get<UpdateInfo>(result).hasWindowsPackage());
}

void UpdateTest::rejectsInvalidReleaseJson()
{
    const UpdateCheckResult result = GitHubReleaseParser::parse(
        QByteArrayLiteral("not-json"));
    QVERIFY(std::holds_alternative<UpdateFailure>(result));
    QCOMPARE(std::get<UpdateFailure>(result).error,
             UpdateServiceError::InvalidResponse);
}

void UpdateTest::rejectsInvalidVersionTag()
{
    const UpdateCheckResult result = GitHubReleaseParser::parse(
        QByteArrayLiteral(R"({"tag_name":"latest","assets":[]})"));
    QVERIFY(std::holds_alternative<UpdateFailure>(result));
    QCOMPARE(std::get<UpdateFailure>(result).error,
             UpdateServiceError::InvalidResponse);
}

QTEST_APPLESS_MAIN(UpdateTest)

#include "UpdateTest.moc"
