#include "MarkdownViewer.h"
#include "MarkdownViewer/md4c/md4c-html.h"

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

MarkdownViewer::MarkdownViewer(const QString &path, QWidget *parent)
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
        }
        QTextBrowser h1 { color: #ffffff; font-size: 24px; font-weight: bold; }
        QTextBrowser h2 { color: #ffffff; font-size: 20px; font-weight: bold; }
        QTextBrowser h3 { color: #ffffff; font-size: 18px; font-weight: bold; }
        QTextBrowser h4 { color: #ffffff; font-size: 16px; font-weight: bold; }
        QTextBrowser h5 { color: #ffffff; font-size: 14px; font-weight: bold; }
        QTextBrowser h6 { color: #ffffff; font-size: 12px; font-weight: bold; }
        QTextBrowser p { color: #ffffff; font-size: 14px; }
        QTextBrowser code { 
            background-color: #333333;
            color: #ffffff;
            font-family: monospace;
            padding: 2px 4px;
            border-radius: 3px;
        }
        QTextBrowser pre {
            background-color: #333333;
            color: #ffffff;
            font-family: monospace;
            padding: 10px;
            border-radius: 5px;
            white-space: pre-wrap;
        }
        QTextBrowser a { color: #4da6ff; }
        QTextBrowser blockquote {
            border-left: 4px solid #666666;
            padding-left: 10px;
            margin-left: 0;
            color: #cccccc;
        }
        QTextBrowser ul, QTextBrowser ol { margin-left: 20px; }
        QTextBrowser li { margin-bottom: 5px; }
        QTextBrowser table { border-collapse: collapse; width: 100%; margin: 10px 0; }
        QTextBrowser th, QTextBrowser td { 
            border: 1px solid #666666; 
            padding: 8px; 
            text-align: left;
        }
        QTextBrowser th { background-color: #333333; }
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
    connect(closeButton, &QPushButton::clicked, this, &MarkdownViewer::close);

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

    // 加载 Markdown 文件
    if (!loadMarkdownFile()) {
        textBrowser->setHtml("<p style='color: red;'>无法加载 Markdown 文件</p>");
    }
}

QString MarkdownViewer::buttonStyle() const {
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

bool MarkdownViewer::loadMarkdownFile() {
    QFile file(currentPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QByteArray markdownData = file.readAll();
    file.close();

    convertMarkdownToHtml(markdownData);
    return true;
}

void MarkdownViewer::convertMarkdownToHtml(const QByteArray &markdown) {
    htmlOutput.clear();

    // 使用md4c-html将Markdown转换为HTML
    int result = md_html(
        markdown.constData(), 
        markdown.size(),
        htmlOutputCallback,
        this,
        0,  // parser flags
        MD_HTML_FLAG_SKIP_UTF8_BOM  // renderer flags
    );

    if (result != 0) {
        textBrowser->setHtml("<p style='color: red;'>Markdown 解析错误</p>");
        return;
    }

    // 设置HTML内容
    QString htmlContent = QString::fromUtf8(htmlOutput);
    textBrowser->setHtml(htmlContent);
}

void MarkdownViewer::htmlOutputCallback(const MD_CHAR *html, MD_SIZE size, void *userdata) {
    MarkdownViewer *viewer = static_cast<MarkdownViewer *>(userdata);
    viewer->htmlOutput.append(html, size);
}

void MarkdownViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);

    // 将关闭按钮放置在右上角
    if (closeButton) {
        closeButton->move(width() - closeButton->width() - 5, 5);
    }
}

