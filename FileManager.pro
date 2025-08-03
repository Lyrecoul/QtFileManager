QT += core gui widgets multimedia multimediawidgets

SOURCES += \
    main.cpp \
    FileManagerWindow.cpp \
    ImageViewer.cpp \
    MarkdownViewer.cpp \
    TextViewer.cpp \
    FileItemDelegate.cpp \
    VirtualKeyboardWidget.cpp \
    MarkdownViewer/md4c/md4c.c \
    MarkdownViewer/md4c/md4c-html.c \
    MarkdownViewer/md4c/entity.c \
    CHighlighter.cpp \
    JsonHighlighter.cpp \
    ToggleSwitch.cpp

HEADERS += \
    FileManagerWindow.h \
    ImageViewer.h \
    MarkdownViewer.h \
    TextViewer.h \
    MyListWidget.h \
    VirtualKeyboardWidget.h \
    FileItemDelegate.h \
    MarkdownViewer/md4c/md4c.h \
    MarkdownViewer/md4c/md4c-html.h \
    MarkdownViewer/md4c/entity.h \
    CHighlighter.h \
    JsonHighlighter.h \
    ToggleSwitch.h

QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -std=gnu++11

INCLUDEPATH += $$PWD/include

LIBS += -L/home/lyrecoul/PenDevelopment/lib \
        -ldrm -lpcre -lgbm -lwayland-client -lwayland-server -lffi \
        -L$$PWD/libs -ldobby -ldl

RESOURCES += resources.qrc

TARGET = FileManager
TEMPLATE = lib

CONFIG += shared
CONFIG += plugin

QMAKE_CXXFLAGS += -fPIC
