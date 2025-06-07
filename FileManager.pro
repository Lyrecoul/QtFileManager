QT += core gui widgets multimedia multimediawidgets

SOURCES += \
    main.cpp \
    FileManagerWindow.cpp \
    ImageViewer.cpp

HEADERS += \
    FileManagerWindow.h \
    ImageViewer.h

CONFIG += c++11
LIBS += -L/home/lyrecoul/PenDevelopment/lib \
        -ldrm -lpcre -lgbm -lwayland-client -lwayland-server -lffi

RESOURCES += resources.qrc
