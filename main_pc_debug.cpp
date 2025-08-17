#include "FileManagerWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDebug>

// PC 调试模式下直接运行的程序
int main(int argc, char *argv[])
{
    // 创建 QApplication 实例
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("FileManager");
    app.setApplicationVersion("1.0");

    // 命令行解析器
    QCommandLineParser parser;
    parser.setApplicationDescription("文件管理器 - PC 调试版本");
    parser.addHelpOption();
    parser.addVersionOption();

    // 添加路径选项
    QCommandLineOption pathOption(QStringList() << "p" << "path",
                                  QCoreApplication::translate("main", "打开指定路径"),
                                  QCoreApplication::translate("main", "路径"));
    parser.addOption(pathOption);

    // 解析命令行参数
    parser.process(app);

    // 创建并显示主窗口
    FileManagerWindow mainWindow;

    // 设置窗口属性
    mainWindow.setAttribute(Qt::WA_QuitOnClose, true);
    mainWindow.resize(800, 600);  // PC 屏幕较大，使用更大的窗口尺寸
    mainWindow.setWindowTitle("文件管理器 - PC 调试版本");

    // 如果指定了路径参数，尝试打开该路径
    if (parser.isSet(pathOption)) {
        QString path = parser.value(pathOption);
        qDebug() << "尝试打开路径:" << path;
        // 这里可以添加设置路径的代码，需要在 FileManagerWindow 中添加相应方法
    }

    // 显示窗口
    mainWindow.show();

    // 运行 Qt 事件循环
    qDebug() << "文件管理器启动成功";
    return app.exec();
}
