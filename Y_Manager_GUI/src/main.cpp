#include <QApplication>
#include <QFont>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    // 高DPI支持 - 必须在QApplication创建前设置
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("Simple Files Compressor");
    app.setApplicationVersion("2.1.0");
    app.setOrganizationName("YONAGI");

    // 设置全局字体 - 使用系统默认字体但启用抗锯齿
    QFont font = app.font();
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferFullHinting);
    app.setFont(font);

    MainWindow window;
    window.show();

    return app.exec();
}
