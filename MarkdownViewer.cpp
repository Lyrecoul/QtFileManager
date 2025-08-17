#include "MarkdownViewer.h"
#include "MarkdownViewer/md4c/md4c-html.h"
#include "qglobal.h"
#include "qnamespace.h"
#include <QStringBuilder> // 新增：字符串拼接优化头文件

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScroller>
#include <QVBoxLayout>

MarkdownViewer::MarkdownViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path), tocVisible(false) {
  setWindowTitle(QFileInfo(path).fileName());
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_DeleteOnClose);

  // 样式表（保持不变）
  setStyleSheet(R"(
        QWidget {
            background-color: #000000;
            color: #ffffff;
        }
        QTextBrowser {
            background-color: #000000;
            color: #ffffff;
            border: none;
            padding: 8px 42px 8px 8px;
            font-size: 14px;
            font-weight: normal;
            font-family: "Microsoft YaHei", "微软雅黑", "Noto Sans SC", "Arial", sans-serif;
            letter-spacing: -0.5px;
        }
        QTextBrowser h1 { color: #ffffff; font-size: 20px; font-weight: bold; margin-top: 8px; margin-bottom: 4px; }
        QTextBrowser h2 { color: #ffffff; font-size: 18px; font-weight: bold; margin-top: 6px; margin-bottom: 3px; }
        QTextBrowser h3 { color: #ffffff; font-size: 16px; font-weight: bold; margin-top: 5px; margin-bottom: 2px; }
        QTextBrowser h4 { color: #ffffff; font-size: 15px; font-weight: bold; margin-top: 4px; margin-bottom: 2px; }
        QTextBrowser h5 { color: #ffffff; font-size: 14px; font-weight: bold; margin-top: 3px; margin-bottom: 1px; }
        QTextBrowser h6 { color: #ffffff; font-size: 13px; font-weight: bold; margin-top: 2px; margin-bottom: 1px; }
        QTextBrowser p { color: #ffffff; font-size: 14px; font-weight: normal; margin: 2px 0; }
        QTextBrowser a { color: #4da6ff; font-weight: normal; }
        QTextBrowser blockquote {
            border-left: 4px solid #666666;
            padding-left: 10px;
            margin-left: 0;
            color: #cccccc;
            font-size: 14px;
        }
        QTextBrowser ul, QTextBrowser ol { margin-left: 15px; }
        QTextBrowser li { margin-bottom: 3px; font-size: 14px; font-weight: normal; }
        QTextBrowser table {
            border-collapse: collapse;
            width: 100%;
            margin: 8px 0;
            font-size: 14px;
        }
        QTextBrowser th, QTextBrowser td {
            border: 1px solid #666666;
            padding: 6px;
            text-align: left;
            min-width: 40px;
            font-size: 14px;
            font-weight: normal;
        }
        QTextBrowser th { background-color: #333333; font-weight: bold; }
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
            font-size: 12px;
            font-weight: normal;
            letter-spacing: -0.5px;
        }
        #tocBrowser a {
            color: #cccccc;
            text-decoration: none;
            font-weight: normal;
        }
        #tocBrowser a:hover {
            color: #ffffff;
        }
    )");

  // 创建文本浏览器（保持不变）
  textBrowser = new QTextBrowser(this);
  textBrowser->setFrameStyle(QFrame::NoFrame);
  textBrowser->setOpenLinks(false);
  textBrowser->setOpenExternalLinks(true);
  textBrowser->setTextInteractionFlags(Qt::NoTextInteraction);

  // 创建关闭按钮（保持不变）
  closeButton = new QPushButton("✕", this);
  closeButton->setVisible(true);
  closeButton->setStyleSheet(buttonStyle());
  closeButton->setFixedSize(30, 30);
  connect(closeButton, &QPushButton::clicked, this, &MarkdownViewer::close);

  // 创建目录按钮（保持不变）
  tocButton = new QPushButton("☰", this);
  tocButton->setVisible(true);
  tocButton->setStyleSheet(buttonStyle());
  tocButton->setFixedSize(30, 30);
  connect(tocButton, &QPushButton::clicked, this, [this]() {
    if (tocVisible) {
      hideTableOfContents();
    } else {
      showTableOfContents();
    }
  });

  // 创建目录面板（保持不变）
  tocPanel = new QWidget(this);
  tocPanel->setObjectName("tocPanel");
  tocPanel->setVisible(false);

  // 创建目录浏览器（保持不变）
  tocBrowser = new QTextBrowser(tocPanel);
  tocBrowser->setObjectName("tocBrowser");
  tocBrowser->setFrameStyle(QFrame::NoFrame);
  tocBrowser->setOpenLinks(false);
  tocBrowser->setOpenExternalLinks(false);
  tocBrowser->setTextInteractionFlags(Qt::NoTextInteraction);

  // 目录面板布局（保持不变）
  QVBoxLayout *tocLayout = new QVBoxLayout(tocPanel);
  tocLayout->setContentsMargins(5, 5, 5, 5);
  tocLayout->addWidget(tocBrowser);

  // 主布局（保持不变）
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
  mainLayout->addWidget(textBrowser);

  // 禁用滚动条（保持不变）
  textBrowser->verticalScrollBar()->setStyleSheet("QScrollBar { width: 0px; }");
  textBrowser->horizontalScrollBar()->setStyleSheet(
      "QScrollBar { height: 0px; }");
  tocBrowser->verticalScrollBar()->setStyleSheet("QScrollBar { width: 0px; }");
  tocBrowser->horizontalScrollBar()->setStyleSheet(
      "QScrollBar { height: 0px; }");

  // 启用触摸滑动支持（保持不变）
  QScroller *scroller = QScroller::scroller(textBrowser);
  QScroller::grabGesture(textBrowser, QScroller::TouchGesture);

  QScroller *tocScroller = QScroller::scroller(tocBrowser);
  QScroller::grabGesture(tocBrowser, QScroller::TouchGesture);

  QScrollerProperties properties = scroller->scrollerProperties();
  QVariant decelerationFactor = 0.25;
  QVariant velocity = 0.1;
  properties.setScrollMetric(QScrollerProperties::DecelerationFactor,
                             decelerationFactor);
  properties.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.2);
  properties.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor,
                             0.8);
  properties.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
  properties.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
  properties.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor,
                             0.5);
  properties.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor,
                             0.2);
  scroller->setScrollerProperties(properties);
  tocScroller->setScrollerProperties(properties);

  resize(320, 170);

  // 新增：初始化正则表达式（预编译）
  codeInlineRegex = QRegularExpression("<code>([^<]+)</code>");
  codeBlockRegex =
      QRegularExpression("<pre><code(.*?)>([\\s\\S]*?)</code></pre>");
  tableRegex = QRegularExpression("<table>");
  thRegex = QRegularExpression("<th>");
  tdRegex = QRegularExpression("<td>");
  wikiImageRegex = QRegularExpression(
      "!\\[\\[([^\\]]+\\.(png|jpg|jpeg|gif|bmp|svg|webp))\\]\\]");
  imgSrcRegex =
      QRegularExpression("<img\\s+[^>]*src\\s*=\\s*['\"]([^'\"]+)['\"][^>]*>");
  imgTagRegex = QRegularExpression("<img([^>]*?)>");

  // 新增：初始化 resize 延迟定时器
  resizeTimer.setSingleShot(true);
  resizeTimer.setInterval(50); // 50ms 延迟，避免频繁触发
  connect(&resizeTimer, &QTimer::timeout, this,
          &MarkdownViewer::updateUIOnResize);

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
            border-radius: 15px;
            width: 30px;
            height: 30px;
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

  // 优化：分块读取（替代 readAll()，适合大文件）
  QByteArray markdownData;
  while (!file.atEnd()) {
    markdownData.append(file.read(4096)); // 每次读取 4KB
  }
  file.close();

  // 新增：缓存当前目录路径
  QFileInfo fileInfo(currentPath);
  currentDirPath = fileInfo.absolutePath();

  extractTableOfContents(markdownData);
  convertMarkdownToHtml(markdownData);
  return true;
}

/* 工具函数 */

// 定义常量，便于维护
const int MAX_SEARCH_LEVELS = 10;

// 辅助函数：解码URL编码的路径
QString decodePath(const QString &encodedPath) {
  return QUrl::fromPercentEncoding(encodedPath.toUtf8());
}

// 辅助函数：构建清理后的绝对路径
QString buildCleanAbsolutePath(const QDir &baseDir,
                               const QString &relativePath) {
  QString absolutePath = baseDir.absoluteFilePath(relativePath);
  qDebug() << "Absolute Path: " << absolutePath;
  return baseDir.cleanPath(absolutePath);
}

// 辅助函数：替换图片标签中的路径
void replaceImageTag(QString &htmlContent, const QString &originalTag,
                     const QString &originalSrc, const QString &newSrc) {
  QString newImgTag = originalTag;
  newImgTag.replace(originalSrc, newSrc);
  htmlContent.replace(originalTag, newImgTag);
}

// 处理Obsidian风格的绝对路径（以/开头）
void handleObsidianAbsolutePath(QString &htmlContent, const QString &imgTag,
                                const QString &srcPath,
                                const QDir &currentDir) {
  QString pathWithoutSlash = srcPath.mid(1); // 去掉开头的'/'
  QStringList pathParts = pathWithoutSlash.split('/', Qt::SkipEmptyParts);

  if (pathParts.isEmpty()) {
    // 路径为空或只有"/"
    QString absolutePath = buildCleanAbsolutePath(currentDir, pathWithoutSlash);
    absolutePath = decodePath(absolutePath);
    replaceImageTag(htmlContent, imgTag, srcPath, absolutePath);
    return;
  }

  // 处理路径部分
  QString targetDirName = decodePath(pathParts.first());
  QDir searchDir(currentDir);
  QString baseDirPath;
  bool foundTargetDir = false;

  // 向上查找目标目录
  for (int i = 0; i < MAX_SEARCH_LEVELS; ++i) {
    if (searchDir.dirName() == targetDirName) {
      baseDirPath = searchDir.absolutePath();
      foundTargetDir = true;
      break;
    }
    if (!searchDir.cdUp())
      break; // 到达文件系统根目录
  }

  // 构建新路径
  QString absolutePath;
  if (foundTargetDir) {
    // 计算剩余路径
    int targetDirPos = pathWithoutSlash.indexOf(pathParts.first());
    QString remainingPath;
    if (targetDirPos >= 0) {
      remainingPath =
          pathWithoutSlash.mid(targetDirPos + pathParts.first().length() + 1);
    } else {
      remainingPath = pathWithoutSlash;
    }

    absolutePath = buildCleanAbsolutePath(QDir(baseDirPath), remainingPath);
  } else {
    // 未找到目标目录，使用当前目录作为基准
    absolutePath = buildCleanAbsolutePath(currentDir, pathWithoutSlash);
  }

  absolutePath = decodePath(absolutePath);
  replaceImageTag(htmlContent, imgTag, srcPath, absolutePath);
}

// 处理普通相对路径
void handleRelativePath(QString &htmlContent, const QString &imgTag,
                        const QString &srcPath, const QDir &currentDir) {
  QString absolutePath = buildCleanAbsolutePath(currentDir, srcPath);
  absolutePath = decodePath(absolutePath);
  replaceImageTag(htmlContent, imgTag, srcPath, absolutePath);
}
/* 工具函数结束 */

void MarkdownViewer::convertMarkdownToHtml(const QByteArray &markdown) {
  htmlOutput.clear();

  int result =
      md_html(markdown.constData(), markdown.size(), htmlOutputCallback, this,
              MD_FLAG_TABLES, MD_HTML_FLAG_SKIP_UTF8_BOM);

  if (result != 0) {
    textBrowser->setHtml("<p style='color: red;'>Markdown 解析错误</p>");
    return;
  }

  QString htmlContent = QString::fromUtf8(htmlOutput);

  // 优化：使用预编译的正则表达式
  htmlContent.replace(
      codeInlineRegex,
      "<code style=\"background-color:#333333;color:#ffea00;font-family: "
      "\"Microsoft YaHei\", \"微软雅黑\", \"Noto Sans SC\", "
      "monospace;padding:2px 4px;font-size:13px;\">\\1</code>");

  htmlContent.replace(
      codeBlockRegex,
      "<pre style=\"background-color:#222222;color:#ffea00;font-family: "
      "\"Microsoft YaHei\", \"微软雅黑\", \"Noto Sans SC\", "
      "monospace;padding:8px;font-size:13px;margin:6px 0;\">\\2</pre>");

  htmlContent.replace(
      tableRegex,
      "<table style=\"border-collapse:collapse;width:100%;margin:10px "
      "0;font-size:16px;\">");
  htmlContent.replace(thRegex, "<th style=\"border:1px solid "
                               "#666666;padding:8px;text-align:left;min-width:"
                               "40px;background-color:#333333;\">");
  htmlContent.replace(tdRegex,
                      "<td style=\"border:1px solid "
                      "#666666;padding:8px;text-align:left;min-width:40px;\">");

  // 处理 Wiki 图片格式
  htmlContent.replace(wikiImageRegex, "<img src=\"\\1\" alt=\"\\1\">");

  // 优化：使用缓存的 currentDirPath 处理图片路径
  QRegularExpressionMatchIterator i = imgSrcRegex.globalMatch(htmlContent);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString imgTag = match.captured(0);
    QString srcPath = match.captured(1);

    // 只处理非HTTP/HTTPS的路径
    if (!srcPath.startsWith("http://") && !srcPath.startsWith("https://")) {
      QDir currentDir(currentDirPath);

      if (srcPath.startsWith('/')) {
        // 处理Obsidian绝对路径
        handleObsidianAbsolutePath(htmlContent, imgTag, srcPath, currentDir);
      } else {
        // 处理普通相对路径
        handleRelativePath(htmlContent, imgTag, srcPath, currentDir);
      }
    }
  }

  // 强制图片高度
  htmlContent.replace(imgTagRegex, "<img\\1 height=\"200\" >");

  textBrowser->setHtml(htmlContent);
}

void MarkdownViewer::htmlOutputCallback(const MD_CHAR *html, MD_SIZE size,
                                        void *userdata) {
  MarkdownViewer *viewer = static_cast<MarkdownViewer *>(userdata);
  viewer->htmlOutput.append(html, size);
}

void MarkdownViewer::extractTableOfContents(const QByteArray &markdown) {
  tocHeadings.clear();
  QString markdownText = QString::fromUtf8(markdown);

  QRegularExpression re("^(#{1,6})\\s+(.+)$",
                        QRegularExpression::MultilineOption);
  QRegularExpressionMatchIterator i = re.globalMatch(markdownText);

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString level = match.captured(1);
    QString title = match.captured(2);
    tocHeadings.append(QString::number(level.length()) + "|" + title);
  }
}

void MarkdownViewer::showTableOfContents() {
  if (tocHeadings.isEmpty()) {
    return;
  }

  // 优化：使用QStringBuilder（%操作符）加速字符串拼接
  QString tocHtml = QStringLiteral(
      "<div style='padding: 5px;'><h3 style='margin-top: 0px;'>目录</h3>");

  for (const QString &heading : tocHeadings) {
    QStringList parts = heading.split("|");
    if (parts.size() >= 2) {
      int level = parts[0].toInt();
      QString title = parts[1];
      QString indent = QString(" ").repeated((level - 1) * 2);

      // 用%替代+，减少内存分配
      tocHtml = tocHtml % "<div style='margin: 3px 0;'" % indent %
                "<a href='#" % QString(title).replace(" ", "_") %
                "' style='color: #cccccc; text-decoration: none;'>" % title %
                "</a></div>";
    }
  }

  tocHtml = tocHtml % "</div>";
  tocBrowser->setHtml(tocHtml);
  tocPanel->setVisible(true);
  tocVisible = true;

  // 触发一次 UI 更新
  updateUIOnResize();
}

void MarkdownViewer::hideTableOfContents() {
  tocPanel->setVisible(false);
  tocVisible = false;
}

// 优化：延迟处理 resize 事件
void MarkdownViewer::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  resizeTimer.start(); // 每次 resize 触发定时器，延迟执行更新
}

// 新增：实际执行 UI 调整的函数
void MarkdownViewer::updateUIOnResize() {
  if (closeButton) {
    closeButton->move(width() - closeButton->width() - 5, 5);
  }
  if (tocButton) {
    tocButton->move(width() - tocButton->width() - 5,
                    closeButton->y() + closeButton->height() + 5);
  }
  if (tocPanel && tocVisible) {
    int panelWidth = qMin(static_cast<int>(width() * 0.8), 250);
    int panelHeight = qMin(static_cast<int>(height() * 0.7), 300);
    tocPanel->setGeometry(5, 5, panelWidth, panelHeight);
  }
}
