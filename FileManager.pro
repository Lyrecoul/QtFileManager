QT += core gui widgets multimedia multimediawidgets

SOURCES += \
    main.cpp \
    FileManagerWindow.cpp \
    ImageViewer.cpp \
    MarkdownViewer.cpp \
    FileItemDelegate.cpp \
    VirtualKeyboardWidget.cpp \
    MarkdownViewer/md4c/md4c.c \
    MarkdownViewer/md4c/md4c-html.c \
    MarkdownViewer/md4c/entity.c

HEADERS += \
    FileManagerWindow.h \
    ImageViewer.h \
    MarkdownViewer.h \
    MyListWidget.h \
    VirtualKeyboardWidget.h \
    FileItemDelegate.h \
    MarkdownViewer/md4c/md4c.h \
    MarkdownViewer/md4c/md4c-html.h \
    MarkdownViewer/md4c/entity.h

QMAKE_CFLAGS += -std=c99
QMAKE_CXXFLAGS += -std=gnu++11

LIBS += -L/home/lyrecoul/PenDevelopment/lib \
        -ldrm -lpcre -lgbm -lwayland-client -lwayland-server -lffi

RESOURCES += resources.qrc
