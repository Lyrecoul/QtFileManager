# PC 调试专用的项目文件
# 这个项目文件针对 PC 环境进行了优化，移除了 ARM 架构相关的代码

QT += core gui widgets multimedia multimediawidgets

# 添加调试信息
CONFIG += debug
CONFIG += c++11

# 输出为可执行文件而非共享库
TEMPLATE = app
TARGET = FileManager_PC_Debug

# 定义宏，禁用 ARM 架构特定代码
DEFINES += PC_DEBUG_MODE

# 源文件
SOURCES += \
    main_pc_debug.cpp \
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

# 头文件
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

# 包含路径
INCLUDEPATH += $$PWD/include

# PC 环境下不需要的库
# LIBS += -L/home/lyrecoul/PenDevelopment/lib \
#         -ldrm -lpcre -lgbm -lwayland-client -lwayland-server -lffi \
#         -L$$PWD/libs -ldobby -ldl

# 资源文件
RESOURCES += resources.qrc

# PC 调试特定设置
QMAKE_CXXFLAGS_DEBUG += -O0 -g3 -Wall -Wextra
QMAKE_LFLAGS_DEBUG +=

# 输出目录
DESTDIR = $$PWD/build_pc_debug

# 创建输出目录
!exists($$DESTDIR) {
    system(mkdir -p $$DESTDIR)
}

# 安装规则
target.path = $$DESTDIR
INSTALLS += target
