#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc,argv);

    QCoreApplication::setOrganizationName("StuKit");
    QCoreApplication::setApplicationName("StuKit");

    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [](){QCoreApplication::exit(-1);},
        Qt::QueuedConnection
    );

    engine.loadFromModule("StuKit","Main");

    return app.exec();
}
