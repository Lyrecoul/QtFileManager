#include "TextViewer.h"

#include <QApplication>
#include <QFileInfo>
#include <QEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextBrowser>
#include <QFile>
#include <QByteArray>
#include <QShowEvent>
#include <QResizeEvent>
#include <QScroller>
#include <QScrollArea>
#include <QFontDatabase>

TextViewer::TextViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path) {
    setWindowTitle(QFileInfo(path).fileName());
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // 设置样式
    setStyleSheet(R"(
        QWidget {
            background-color: #000000;
            color: #ffffff;
        }
        QTextBrowser {
            background-color: #000000;
            color: #ffffff;
            border: none;
            padding: 10px;
        }
        QTextBrowser, QTextEdit, QPlainTextEdit {
            font-family: "Microsoft YaHei", "微软雅黑";
            font-size: 14px;
        }
        QTextBrowser a { color: #4da6ff; }
    )");

    // 创建文本浏览器
    textBrowser = new QTextBrowser(this);
    textBrowser->setFrameStyle(QFrame::NoFrame);
    textBrowser->setOpenLinks(false);
    textBrowser->setOpenExternalLinks(true);
    // 设置文本不可选中
    textBrowser->setTextInteractionFlags(Qt::NoTextInteraction);

    // 创建关闭按钮
    closeButton = new QPushButton("✕", this);
    closeButton->setVisible(true);
    closeButton->setStyleSheet(buttonStyle());
    closeButton->setFixedSize(32, 32);
    connect(closeButton, &QPushButton::clicked, this, &TextViewer::close);

    // 设置布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(textBrowser);

    // 禁用滚动条
    textBrowser->verticalScrollBar()->setStyleSheet("QScrollBar { width: 0px; }");
    textBrowser->horizontalScrollBar()->setStyleSheet("QScrollBar { height: 0px; }");

    // 启用触摸滑动支持
    QScroller::grabGesture(textBrowser, QScroller::TouchGesture);

    // 设置窗口大小为 320x170
    resize(320, 170);

    // 加载文本文件
    if (!loadTextFile()) {
        textBrowser->setHtml("<p style='color: red;'>无法加载文本文件</p>");
    }
}

QString TextViewer::buttonStyle() const {
    return R"(
        QPushButton {
            color: white;
            font-size: 16px;
            font-weight: bold;
            background: #353535;
            border-radius: 16px;
            width: 32px;
            height: 32px;
            border: none;
            outline: none;
        }
        QPushButton:hover {
            background: #555555;
            border: 1px solid #777777;
        }
        QPushButton:pressed {
            background: #222222;
            border: 1px solid #444444;
        }
    )";
}

bool TextViewer::loadTextFile() {
    QFile file(currentPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray textData = file.readAll();
    file.close();

    // 直接设置文本内容
    textBrowser->setPlainText(QString::fromUtf8(textData));
    return true;
}

void TextViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // 将关闭按钮放置在右上角
    if (closeButton) {
        closeButton->move(width() - closeButton->width() - 5, 5);
    }
}
