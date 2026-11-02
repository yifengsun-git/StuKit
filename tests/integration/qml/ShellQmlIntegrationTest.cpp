#include <QCoreApplication>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace {

std::unique_ptr<QObject> createHomePage(
    QQmlEngine &engine,
    qreal width,
    qreal height = 720)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitShellTest"),
        QStringLiteral("HomePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }

    std::unique_ptr<QObject> page(component.create());
    if (!page) {
        qWarning().noquote() << component.errorString();
        return {};
    }

    page->setProperty("width", width);
    page->setProperty("height", height);
    QCoreApplication::processEvents();
    return page;
}

std::unique_ptr<QObject> createAppearancePage(QQmlEngine &engine, qreal width)
{
    QQmlComponent component(&engine);
    component.loadFromModule(
        QStringLiteral("StuKitShellTest"),
        QStringLiteral("AppearancePage"));
    if (!component.isReady()) {
        qWarning().noquote() << component.errorString();
    }

    const QVariantList themes{
        QVariantMap{
            {QStringLiteral("themeId"), QStringLiteral("default")},
            {QStringLiteral("name"), QStringLiteral("默认")},
            {QStringLiteral("description"), QStringLiteral("默认主题")},
            {QStringLiteral("author"), QStringLiteral("StuKit")},
            {QStringLiteral("version"), QStringLiteral("1.0.0")},
            {QStringLiteral("builtIn"), true},
            {QStringLiteral("primaryColor"), QStringLiteral("#2563EB")},
            {QStringLiteral("backgroundColor"), QStringLiteral("#F3F4F6")},
            {QStringLiteral("surfaceColor"), QStringLiteral("#FFFFFF")},
        },
        QVariantMap{
            {QStringLiteral("themeId"), QStringLiteral("dark")},
            {QStringLiteral("name"), QStringLiteral("深色")},
            {QStringLiteral("description"), QStringLiteral("深色主题")},
            {QStringLiteral("author"), QStringLiteral("StuKit")},
            {QStringLiteral("version"), QStringLiteral("1.0.0")},
            {QStringLiteral("builtIn"), true},
            {QStringLiteral("primaryColor"), QStringLiteral("#60A5FA")},
            {QStringLiteral("backgroundColor"), QStringLiteral("#111827")},
            {QStringLiteral("surfaceColor"), QStringLiteral("#1F2937")},
        },
    };
    const QVariantMap viewModel{
        {QStringLiteral("themeId"), QStringLiteral("default")},
        {QStringLiteral("themes"), themes},
        {QStringLiteral("currentTheme"), themes.constFirst()},
        {QStringLiteral("hasError"), false},
        {QStringLiteral("errorMessage"), QString{}},
        {QStringLiteral("statusMessage"), QString{}},
        {QStringLiteral("hasPendingTheme"), false},
        {QStringLiteral("pendingTheme"), QVariantMap{}},
        {QStringLiteral("pendingThemeReplacesExisting"), false},
    };
    std::unique_ptr<QObject> page(component.createWithInitialProperties({
        {QStringLiteral("viewModel"), viewModel},
    }));
    if (!page) {
        qWarning().noquote() << component.errorString();
        return {};
    }

    page->setProperty("width", width);
    page->setProperty("height", 720);
    QCoreApplication::processEvents();
    return page;
}

} // namespace

class ShellQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void homePageAdaptsToCompactAndDesktopWidths();
    void featureCardsForwardNavigationRequests();
    void appearancePageShowsAvailableThemes();
};

void ShellQmlIntegrationTest::homePageAdaptsToCompactAndDesktopWidths()
{
    QQmlEngine engine;
    auto page = createHomePage(engine, 360, 640);
    QVERIFY(page);

    QObject *grid = page->findChild<QObject *>(
        QStringLiteral("homeFeatureGrid"));
    QObject *accountingCard = page->findChild<QObject *>(
        QStringLiteral("accountingFeatureCard"));
    QObject *todaySnapshot = page->findChild<QObject *>(
        QStringLiteral("homeTodaySnapshot"));
    QVERIFY(grid);
    QVERIFY(accountingCard);
    QVERIFY(todaySnapshot);

    QCOMPARE(page->property("compactLayout").toBool(), true);
    QCOMPARE(page->property("featureColumns").toInt(), 1);
    QTRY_VERIFY(accountingCard->property("width").toReal() > 0.0);
    QVERIFY(accountingCard->property("width").toReal() <= 360.0);
    QVERIFY(grid->property("width").toReal() <= 360.0);
    QCOMPARE(todaySnapshot->property("visible").toBool(), false);

    page->setProperty("width", 960.0);
    QCoreApplication::processEvents();

    QTRY_COMPARE(page->property("compactLayout").toBool(), false);
    QTRY_COMPARE(page->property("featureColumns").toInt(), 2);
    QTRY_COMPARE(todaySnapshot->property("visible").toBool(), true);
    QVERIFY(grid->property("width").toReal() <= 960.0);
}

void ShellQmlIntegrationTest::featureCardsForwardNavigationRequests()
{
    QQmlEngine engine;
    auto page = createHomePage(engine, 360, 640);
    QVERIFY(page);

    struct RequestCase
    {
        const char *cardName;
        const char *signalSignature;
    };

    const RequestCase cases[] = {
        {"accountingFeatureCard", SIGNAL(accountingRequested())},
        {"scheduleFeatureCard", SIGNAL(scheduleRequested())},
        {"tasksFeatureCard", SIGNAL(tasksRequested())},
    };

    for (const RequestCase &requestCase : cases) {
        QObject *card = page->findChild<QObject *>(
            QString::fromLatin1(requestCase.cardName));
        QVERIFY(card);

        QSignalSpy requestSpy(page.get(), requestCase.signalSignature);
        QVERIFY(requestSpy.isValid());
        QVERIFY(QMetaObject::invokeMethod(card, "click"));
        QCOMPARE(requestSpy.count(), 1);
    }

    page->setProperty("width", 960.0);
    QCoreApplication::processEvents();
    struct SnapshotRequestCase
    {
        const char *action;
        const char *signalSignature;
    };
    const SnapshotRequestCase snapshotCases[] = {
        {"schedule", SIGNAL(scheduleRequested())},
        {"tasks", SIGNAL(tasksRequested())},
        {"accounting", SIGNAL(accountingRequested())},
    };
    for (const SnapshotRequestCase &requestCase : snapshotCases) {
        QSignalSpy requestSpy(page.get(), requestCase.signalSignature);
        QVERIFY(requestSpy.isValid());
        QVERIFY(QMetaObject::invokeMethod(
            page.get(), "activateSnapshot",
            Q_ARG(QString, QString::fromLatin1(requestCase.action))));
        QCOMPARE(requestSpy.count(), 1);
    }

    QObject *weatherCard = page->findChild<QObject *>(
        QStringLiteral("weatherFeatureCard"));
    QVERIFY(weatherCard);
    QVERIFY(!weatherCard->property("visible").toBool());
}

void ShellQmlIntegrationTest::appearancePageShowsAvailableThemes()
{
    QQmlEngine engine;
    QQuickWindow window;
    auto page = createAppearancePage(engine, 960);
    QVERIFY(page);
    auto *pageItem = qobject_cast<QQuickItem *>(page.get());
    QVERIFY(pageItem);
    pageItem->setParentItem(window.contentItem());
    window.resize(960, 720);
    window.show();
    QCoreApplication::processEvents();

    QObject *themeRepeater = page->findChild<QObject *>(
        QStringLiteral("appearanceThemeRepeater"));
    QObject *importButton = page->findChild<QObject *>(
        QStringLiteral("appearanceImportThemeButton"));
    QVERIFY(themeRepeater);
    QVERIFY(importButton);
    QVERIFY(page->findChild<QObject *>(QStringLiteral("themePreviewPanel")));
    QTRY_COMPARE(themeRepeater->property("count").toInt(), 2);
    QCOMPARE(page->property("compactLayout").toBool(), false);

    page->setProperty("width", 360);
    QTRY_COMPARE(page->property("compactLayout").toBool(), true);
}

QTEST_MAIN(ShellQmlIntegrationTest)

#include "ShellQmlIntegrationTest.moc"
