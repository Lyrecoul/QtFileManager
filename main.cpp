#include "FileManagerWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  FileManagerWindow *MyWindow = nullptr;

  MyWindow = new FileManagerWindow();

  MyWindow->resize(320, 170);
  MyWindow->move(0, 0);
  MyWindow->show();

  return app.exec();
}
