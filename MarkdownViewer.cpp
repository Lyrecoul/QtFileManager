#include "MarkdownViewer.h"
#include "MarkdownViewer/md4c/md4c-html.h"
#include "qnamespace.h"

#include <QFileInfo>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QFile>
#include <QByteArray>
#include <QResizeEvent>
#include <QScroller>
#include <QPushButton>
#include <QRegularExpression>
#include <QHBoxLayout>

MarkdownViewer::MarkdownViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path), tocVisible(false) {
    setWindowTitle(QFileInfo(path).fileName());
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // 样式表
    setStyleSheet(R"(
        QWidget {
            background-color: #000000;
            color: #ffffff;
        }
        QTextBrowser {
            background-color: #000000;
            color: #ffffff;
            border: none;
            padding: 10px 42px 10px 10px;
            font-size: 18px;
            font-family: "Microsoft YaHei", "微软雅黑", "Noto Sans SC", "Arial", sans-serif;
        }
        QTextBrowser h1 { color: #ffffff; font-size: 28px; font-weight: bold; }
        QTextBrowser h2 { color: #ffffff; font-size: 24px; font-weight: bold; }
        QTextBrowser h3 { color: #ffffff; font-size: 20px; font-weight: bold; }
        QTextBrowser h4 { color: #ffffff; font-size: 18px; font-weight: bold; }
        QTextBrowser h5 { color: #ffffff; font-size: 16px; font-weight: bold; }
        QTextBrowser h6 { color: #ffffff; font-size: 14px; font-weight: bold; }
        QTextBrowser p { color: #ffffff; font-size: 16px; }
        /* 行内代码 */
        code, .inline-code {
            background-color: #333333;
            color: #ffea00;
            font-family: "Fira Mono", "Consolas", "monospace";
            padding: 2px 4px;
            font-size: 16px;
        }
        /* 代码块 */
        pre, .code-block {
            background-color: #222222;
            color: #ffea00;
            font-family: "Fira Mono", "Consolas", "monospace";
            padding: 10px;
            font-size: 16px;
            margin: 8px 0;
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
        QTextBrowser table {
            border-collapse: collapse;
            width: 100%;
            margin: 10px 0;
            font-size: 16px;
        }
        QTextBrowser th, QTextBrowser td {
            border: 1px solid #666666;
            padding: 8px;
            text-align: left;
            min-width: 40px;
        }
        QTextBrowser th { background-color: #333333; }
        /* 目录面板样式 */
        #tocPanel {
            background-color: rgba(30, 30, 30, 240);
            border: 1px solid #555555;
            border-radius: 5px;
        }
        #tocBrowser {
            background-color: transparent;
            border: none;
            padding: 5px;
            font-size: 14px;
        }
        #tocBrowser a {
            color: #cccccc;
            text-decoration: none;
        }
        #tocBrowser a:hover {
            color: #ffffff;
        }
    )");

    // 创建文本浏览器
    textBrowser = new QTextBrowser(this);
    textBrowser->setFrameStyle(QFrame::NoFrame);
    textBrowser->setOpenLinks(false);
    textBrowser->setOpenExternalLinks(true);
    textBrowser->setTextInteractionFlags(Qt::NoTextInteraction);

    // 创建关闭按钮
    closeButton = new QPushButton("✕", this);
    closeButton->setVisible(true);
    closeButton->setStyleSheet(buttonStyle());
    closeButton->setFixedSize(40, 40);
    connect(closeButton, &QPushButton::clicked, this, &MarkdownViewer::close);

    // 创建目录按钮
    tocButton = new QPushButton("☰", this);
    tocButton->setVisible(true);
    tocButton->setStyleSheet(buttonStyle());
    tocButton->setFixedSize(40, 40);
    connect(tocButton, &QPushButton::clicked, this, [this]() {
        if (tocVisible) {
            hideTableOfContents();
        } else {
            showTableOfContents();
        }
    });

    // 创建目录面板
    tocPanel = new QWidget(this);
    tocPanel->setObjectName("tocPanel");
    tocPanel->setVisible(false);

    // 创建目录浏览器
    tocBrowser = new QTextBrowser(tocPanel);
    tocBrowser->setObjectName("tocBrowser");
    tocBrowser->setFrameStyle(QFrame::NoFrame);
    tocBrowser->setOpenLinks(false);
    tocBrowser->setOpenExternalLinks(false);
    tocBrowser->setTextInteractionFlags(Qt::NoTextInteraction);

    // 目录面板布局
    QVBoxLayout *tocLayout = new QVBoxLayout(tocPanel);
    tocLayout->setContentsMargins(5, 5, 5, 5);
    tocLayout->addWidget(tocBrowser);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(textBrowser);

    // 禁用滚动条
    textBrowser->verticalScrollBar()->setStyleSheet("QScrollBar { width: 0px; }");
    textBrowser->horizontalScrollBar()->setStyleSheet("QScrollBar { height: 0px; }");
    tocBrowser->verticalScrollBar()->setStyleSheet("QScrollBar { width: 0px; }");
    tocBrowser->horizontalScrollBar()->setStyleSheet("QScrollBar { height: 0px; }");

    // 启用触摸滑动支持
    QScroller *scroller = QScroller::scroller(textBrowser);
    QScroller::grabGesture(textBrowser, QScroller::TouchGesture);

    QScroller *tocScroller = QScroller::scroller(tocBrowser);
    QScroller::grabGesture(tocBrowser, QScroller::TouchGesture);

    QScrollerProperties properties = scroller->scrollerProperties();
    QVariant decelerationFactor = 0.25;
    QVariant velocity = 0.1;
    properties.setScrollMetric(QScrollerProperties::DecelerationFactor, decelerationFactor);
    properties.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.2);
    properties.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.8);
    properties.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
    properties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
    properties.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.5);
    properties.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.2);
    scroller->setScrollerProperties(properties);
    tocScroller->setScrollerProperties(properties);

    resize(320, 170);

    if (!loadMarkdownFile()) {
        textBrowser->setHtml("<p style='color: red;'>无法加载 Markdown 文件</p>");
    }
}

QString MarkdownViewer::buttonStyle() const {
    return R"(
        QPushButton {
            color: white;
            font-size: 20px;
            font-weight: bold;
            background: #353535;
            border-radius: 20px;
            width: 40px;
            height: 40px;
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

    extractTableOfContents(markdownData);
    convertMarkdownToHtml(markdownData);
    return true;
}

void MarkdownViewer::convertMarkdownToHtml(const QByteArray &markdown) {
    htmlOutput.clear();

    int result = md_html(
        markdown.constData(),
        markdown.size(),
        htmlOutputCallback,
        this,
        MD_FLAG_TABLES,
        MD_HTML_FLAG_SKIP_UTF8_BOM
    );

    if (result != 0) {
        textBrowser->setHtml("<p style='color: red;'>Markdown 解析错误</p>");
        return;
    }

    QString htmlContent = QString::fromUtf8(htmlOutput);

    // 行内代码
    htmlContent.replace(QRegularExpression("<code>([^<]+)</code>"),
        "<code style=\"background-color:#333333;color:#ffea00;font-family:'Fira Mono','Consolas',monospace;padding:2px 4px;font-size:16px;\">\\1</code>");

    // 代码块
    htmlContent.replace(QRegularExpression("<pre><code(.*?)>([\\s\\S]*?)</code></pre>"),
        "<pre style=\"background-color:#222222;color:#ffea00;font-family:'Fira Mono','Consolas',monospace;padding:10px;font-size:16px;margin:8px 0;\">\\2</pre>");

    // 表格
    htmlContent.replace(QRegularExpression("<table>"),
        "<table style=\"border-collapse:collapse;width:100%;margin:10px 0;font-size:16px;\">");
    htmlContent.replace(QRegularExpression("<th>"),
        "<th style=\"border:1px solid #666666;padding:8px;text-align:left;min-width:40px;background-color:#333333;\">");
    htmlContent.replace(QRegularExpression("<td>"),
        "<td style=\"border:1px solid #666666;padding:8px;text-align:left;min-width:40px;\">");

    textBrowser->setHtml(htmlContent);
}

void MarkdownViewer::htmlOutputCallback(const MD_CHAR *html, MD_SIZE size, void *userdata) {
    MarkdownViewer *viewer = static_cast<MarkdownViewer *>(userdata);
    viewer->htmlOutput.append(html, size);
}

void MarkdownViewer::extractTableOfContents(const QByteArray &markdown) {
    tocHeadings.clear();
    QString markdownText = QString::fromUtf8(markdown);

    // 使用正则表达式匹配所有标题（# 标题）
    QRegularExpression re("^(#{1,6})\\s+(.+)$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator i = re.globalMatch(markdownText);

    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString level = match.captured(1);  // 获取标题级别（#的数量）
        QString title = match.captured(2);  // 获取标题文本

        // 存储标题级别和标题文本
        tocHeadings.append(QString::number(level.length()) + "|" + title);
    }
}

void MarkdownViewer::showTableOfContents() {
    if (tocHeadings.isEmpty()) {
        return;
    }

    // 生成目录HTML
    QString tocHtml = "<div style='padding: 5px;'><h3 style='margin-top: 0px;'>目录</h3>";

    for (const QString &heading : tocHeadings) {
        QStringList parts = heading.split("|");
        if (parts.size() >= 2) {
            int level = parts[0].toInt();
            QString title = parts[1];

            // 根据标题级别添加缩进
            QString indent = QString(" ").repeated((level - 1) * 2);

            // 添加到目录HTML
            tocHtml += "<div style='margin: 3px 0;'>" + indent + 
                      "<a href='#" + QString(title).replace(" ", "_") + 
                      "' style='color: #cccccc; text-decoration: none;'>" + 
                      title + "</a></div>";
        }
    }

    tocHtml += "</div>";

    // 设置目录内容并显示面板
    tocBrowser->setHtml(tocHtml);
    tocPanel->setVisible(true);
    tocVisible = true;

    // 更新目录面板位置和大小
    int panelWidth = qMin(static_cast<int>(width() * 0.8), 250);
    int panelHeight = qMin(static_cast<int>(height() * 0.7), 300);
    tocPanel->setGeometry(5, 5, panelWidth, panelHeight);
}

void MarkdownViewer::hideTableOfContents() {
    tocPanel->setVisible(false);
    tocVisible = false;
}

void MarkdownViewer::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    if (closeButton) {
        closeButton->move(width() - closeButton->width() - 5, 5);
    }
    if (tocButton) {
        tocButton->move(width() - tocButton->width() - 5, closeButton->y() + closeButton->height() + 5);
    }
    if (tocPanel && tocVisible) {
        // 目录面板宽度为屏幕宽度的80%，最大宽度为250
        int panelWidth = qMin(static_cast<int>(width() * 0.8), 250);
        // 目录面板高度为屏幕高度的70%，最大高度为300
        int panelHeight = qMin(static_cast<int>(height() * 0.7), 300);
        tocPanel->setGeometry(5, 5, panelWidth, panelHeight);
    }
}