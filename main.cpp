#include <QApplication>
#include "FileManagerWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    FileManagerWindow *MyWindow = nullptr;

    MyWindow = new FileManagerWindow();
    MyWindow->setAttribute(Qt::WA_QuitOnClose, true); 
    MyWindow->showFullScreen();
    MyWindow->activateWindow();
    return app.exec();
}
