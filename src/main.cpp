#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("qMenuEditor");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("qMenuEditor");

    MainWindow window;
    window.show();

    return app.exec();
}
