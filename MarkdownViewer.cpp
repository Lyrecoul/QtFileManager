#include "MarkdownViewer.h"
#include "MarkdownViewer/md4c/md4c-html.h"
#include "qglobal.h"
#include "qnamespace.h"
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScroller>
#include <QStringBuilder>
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
            font-family: "OPPOSans", "Noto Color Emoji", sans-serif;
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
            min-width: 270px;
            min-height: 100px;
        }
        #tocBrowser {
            background-color: transparent;
            border: none;
            padding: 8px;
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

  // 创建文本浏览器（优化交互权限）
  textBrowser = new QTextBrowser(this);
  textBrowser->setFrameStyle(QFrame::NoFrame);
  textBrowser->setOpenLinks(false);
  textBrowser->setOpenExternalLinks(true);
  // 仅允许链接交互，禁止文本选择
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

  // 创建目录浏览器（优化交互权限）
  tocBrowser = new QTextBrowser(tocPanel);
  tocBrowser->setObjectName("tocBrowser");
  tocBrowser->setFrameStyle(QFrame::NoFrame);
  tocBrowser->setOpenLinks(false);
  tocBrowser->setOpenExternalLinks(false);
  // 允许链接交互和触摸交互
  tocBrowser->setTextInteractionFlags(Qt::LinksAccessibleByMouse |
                                      Qt::LinksAccessibleByKeyboard);
  tocBrowser->setAttribute(Qt::WA_AcceptTouchEvents);

  // 连接目录链接点击信号，优化跳转逻辑
  connect(tocBrowser, &QTextBrowser::anchorClicked, this,
          [this](const QUrl &url) {
            QString anchor = url.fragment();
            if (!anchor.isEmpty()) {
              textBrowser->setFocus(); // 确保文本浏览器获得焦点
              textBrowser->scrollToAnchor(
                  anchor); // 仅使用原生锚点滚动（移除冗余光标定位）
            }
          });

  // 为目录浏览器添加触摸事件支持
  tocBrowser->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
  tocBrowser->installEventFilter(this);

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

  // 初始化正则表达式（预编译）
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

  // 初始化 resize 延迟定时器
  resizeTimer.setSingleShot(true);
  resizeTimer.setInterval(50);
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

  // 分块读取（适合大文件）
  QByteArray markdownData;
  while (!file.atEnd()) {
    markdownData.append(file.read(4096));
  }
  file.close();

  // 缓存当前目录路径
  QFileInfo fileInfo(currentPath);
  currentDirPath = fileInfo.absolutePath();

  extractTableOfContents(markdownData);
  convertMarkdownToHtml(markdownData);
  return true;
}

/* 工具函数 */
const int MAX_SEARCH_LEVELS = 10;

// 生成标题锚点 ID（确保唯一性和兼容性）
QString generateAnchorId(const QString &title) {
  QString anchorId = title.trimmed().toLower(); // 统一转为小写，避免大小写问题
  anchorId.replace(" ", "_");
  // 移除特殊字符，只保留安全字符
  anchorId.remove(QRegularExpression("[^a-z0-9_-]"));
  // 确保不以数字开头
  if (!anchorId.isEmpty() && anchorId[0].isDigit()) {
    anchorId.prepend("anchor_");
  }
  // 空标题处理
  if (anchorId.isEmpty()) {
    anchorId = "heading";
  }
  return anchorId;
}

// 解码 URL 编码的路径
QString decodePath(const QString &encodedPath) {
  return QUrl::fromPercentEncoding(encodedPath.toUtf8());
}

// 构建清理后的绝对路径
QString buildCleanAbsolutePath(const QDir &baseDir,
                               const QString &relativePath) {
  QString absolutePath = baseDir.absoluteFilePath(relativePath);
  return baseDir.cleanPath(absolutePath);
}

// 替换图片标签中的路径
void replaceImageTag(QString &htmlContent, const QString &originalTag,
                     const QString &originalSrc, const QString &newSrc) {
  QString newImgTag = originalTag;
  newImgTag.replace(originalSrc, newSrc);
  htmlContent.replace(originalTag, newImgTag);
}

// 为标题添加锚点（优化正则匹配，支持带属性的标题标签）
void addHeadingAnchors(QString &htmlContent) {
  // 匹配带属性的标题标签（如<h1 class="xxx">）
  static const QRegularExpression headingRegex(
      "<h([1-6])(\\s+[^>]*)?>([^<]+)</h\\1>",
      QRegularExpression::CaseInsensitiveOption);

  QRegularExpressionMatchIterator i = headingRegex.globalMatch(htmlContent);
  QList<QPair<int, int>> replacePositions;
  QList<QString> replacements;

  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    int level = match.captured(1).toInt();
    QString title = match.captured(3); // 捕获标题内容
    QString anchorId = generateAnchorId(title);

    // 生成带锚点的标题标签
    QString replacement =
        QString("<h%1 id=\"%2\">%3</h%1>").arg(level).arg(anchorId).arg(title);

    replacePositions.prepend(
        qMakePair(match.capturedStart(), match.capturedLength()));
    replacements.prepend(replacement);
  }

  // 执行替换（从后往前避免索引偏移）
  for (int j = 0; j < replacePositions.size(); ++j) {
    htmlContent.replace(replacePositions[j].first, replacePositions[j].second,
                        replacements[j]);
  }
}

// 处理 Obsidian 风格的绝对路径
void handleObsidianAbsolutePath(QString &htmlContent, const QString &imgTag,
                                const QString &srcPath,
                                const QDir &currentDir) {
  QString pathWithoutSlash = srcPath.mid(1);
  QStringList pathParts = pathWithoutSlash.split('/', Qt::SkipEmptyParts);

  if (pathParts.isEmpty()) {
    QString absolutePath = buildCleanAbsolutePath(currentDir, pathWithoutSlash);
    absolutePath = decodePath(absolutePath);
    replaceImageTag(htmlContent, imgTag, srcPath, absolutePath);
    return;
  }

  QString targetDirName = decodePath(pathParts.first());
  QDir searchDir(currentDir);
  QString baseDirPath;
  bool foundTargetDir = false;

  for (int i = 0; i < MAX_SEARCH_LEVELS; ++i) {
    if (searchDir.dirName() == targetDirName) {
      baseDirPath = searchDir.absolutePath();
      foundTargetDir = true;
      break;
    }
    if (!searchDir.cdUp())
      break;
  }

  QString absolutePath;
  if (foundTargetDir) {
    int targetDirPos = pathWithoutSlash.indexOf(pathParts.first());
    QString remainingPath =
        (targetDirPos >= 0) ? pathWithoutSlash.mid(
                                  targetDirPos + pathParts.first().length() + 1)
                            : pathWithoutSlash;
    absolutePath = buildCleanAbsolutePath(QDir(baseDirPath), remainingPath);
  } else {
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

  // 优化代码块样式
  htmlContent.replace(
      codeInlineRegex,
      "<code style=\"background-color:#333333;color:#ffea00;font-family: "
      "\"Microsoft YaHei\", \"Noto Sans CJK\", \"Noto Sans SC\", "
      "monospace;padding:2px 4px;font-size:13px;\">\\1</code>");

  htmlContent.replace(
      codeBlockRegex,
      "<pre style=\"background-color:#222222;color:#ffea00;font-family: "
      "\"Microsoft YaHei\", \"Noto Sans CJK\", \"Noto Sans SC\", "
      "monospace;padding:8px;font-size:13px;margin:6px 0;\">\\2</pre>");

  // 优化表格样式
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

  // 处理图片路径
  QRegularExpressionMatchIterator i = imgSrcRegex.globalMatch(htmlContent);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString imgTag = match.captured(0);
    QString srcPath = match.captured(1);

    if (!srcPath.startsWith("http://") && !srcPath.startsWith("https://")) {
      QDir currentDir(currentDirPath);
      if (srcPath.startsWith('/')) {
        handleObsidianAbsolutePath(htmlContent, imgTag, srcPath, currentDir);
      } else {
        handleRelativePath(htmlContent, imgTag, srcPath, currentDir);
      }
    }
  }

  // 强制图片高度
  htmlContent.replace(imgTagRegex, "<img\\1 height=\"200\" >");

  // 为标题添加锚点 ID
  addHeadingAnchors(htmlContent);

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

  // 匹配标题行（# 开头）
  static const QRegularExpression re("^(#{1,6})\\s+(.+)$",
                                     QRegularExpression::MultilineOption);
  tocHeadings.reserve(20);

  QRegularExpressionMatchIterator i = re.globalMatch(markdownText);
  while (i.hasNext()) {
    QRegularExpressionMatch match = i.next();
    QString level = match.captured(1);
    QString title = match.captured(2).trimmed();
    QString anchorId = generateAnchorId(title); // 与标题锚点使用相同生成逻辑

    // 存储格式：级别|标题|锚点ID
    tocHeadings.append(QString::number(level.length()) % "|" % title % "|" %
                       anchorId);
  }
}

void MarkdownViewer::showTableOfContents() {
  if (tocHeadings.isEmpty()) {
    return;
  }

  QString tocHtml = QStringLiteral(
      "<div style='padding: 12px;'><h3 style='margin-top: 0px; color: #ffffff; "
      "font-size: 16px; font-weight: bold; border-bottom: 1px solid #555555; "
      "padding-bottom: 8px;'>目录</h3>");

  tocHtml.reserve(tocHeadings.size() * 100);
  for (const QString &heading : tocHeadings) {
    QStringList parts = heading.split("|");
    if (parts.size() >= 3) {
      int level = parts[0].toInt();
      QString title = parts[1];
      QString anchorId = parts[2];

      int indentPixels = (level - 1) * 15;
      QString fontSize = level == 1 ? "13px" : (level == 2 ? "12px" : "11px");
      QString fontWeight = level <= 2 ? "bold" : "normal";

      tocHtml = tocHtml % "<div style='margin: 6px 0; padding-left: " %
                QString::number(indentPixels) % "px;'><a href='#" % anchorId %
                "' style='color: #cccccc; text-decoration: none; font-size: " %
                fontSize % "; font-weight: " % fontWeight % ";'>" % title %
                "</a></div>";
    }
  }

  tocHtml = tocHtml % "</div>";
  tocBrowser->setHtml(tocHtml);
  tocPanel->setVisible(true);
  tocVisible = true;
}

void MarkdownViewer::hideTableOfContents() {
  tocPanel->setVisible(false);
  tocVisible = false;
}

void MarkdownViewer::updateUIOnResize() {
  // 调整按钮位置（根据实际需求补充）
  closeButton->move(width() - 40, 10);
  tocButton->move(width() - 40, 50);
}

void MarkdownViewer::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  resizeTimer.start(); // 延迟处理 resize 事件
}
