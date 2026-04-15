#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("Secure Files Compressor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("YONAGI");

    // 设置UTF-8编码
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    app.setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    MainWindow window;
    window.show();

    return app.exec();
}
