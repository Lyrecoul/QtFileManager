#include <QApplication>
#include "BubbleWidget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    BubbleWidget w;
    w.show();
    return app.exec();
}
