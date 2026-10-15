#include <QCoreApplication>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>

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

} // namespace

class ShellQmlIntegrationTest final : public QObject
{
    Q_OBJECT

private slots:
    void homePageAdaptsToCompactAndDesktopWidths();
    void featureCardsForwardNavigationRequests();
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
    QVERIFY(grid);
    QVERIFY(accountingCard);

    QCOMPARE(page->property("compactLayout").toBool(), true);
    QCOMPARE(page->property("featureColumns").toInt(), 1);
    QTRY_VERIFY(accountingCard->property("width").toReal() > 0.0);
    QVERIFY(accountingCard->property("width").toReal() <= 360.0);
    QVERIFY(grid->property("width").toReal() <= 360.0);

    page->setProperty("width", 960.0);
    QCoreApplication::processEvents();

    QTRY_COMPARE(page->property("compactLayout").toBool(), false);
    QTRY_COMPARE(page->property("featureColumns").toInt(), 2);
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

    QObject *weatherCard = page->findChild<QObject *>(
        QStringLiteral("weatherFeatureCard"));
    QVERIFY(weatherCard);
    QVERIFY(!weatherCard->property("visible").toBool());
}

QTEST_MAIN(ShellQmlIntegrationTest)

#include "ShellQmlIntegrationTest.moc"
