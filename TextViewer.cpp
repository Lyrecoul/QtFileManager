#include "TextViewer.h"
#include "JsonHighlighter.h"
#include "CHighlighter.h"

#include <QApplication>
#include <QByteArray>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QShowEvent>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QSettings>  // 新增：用于保存和恢复阅读进度

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
            padding: 10px 42px 10px 10px; /* 右侧增加32px按钮宽度+10px边距 */
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
  textBrowser->horizontalScrollBar()->setStyleSheet(
      "QScrollBar { height: 0px; }");

  // 启用触摸滑动支持
  QScroller *scroller = QScroller::scroller(textBrowser);
  QScroller::grabGesture(textBrowser, QScroller::TouchGesture);

  // 配置滑动参数，使滑动更加平滑
  QScrollerProperties properties = scroller->scrollerProperties();
  QVariant decelerationFactor = 0.25; // 减速因子，值越小减速越快
  QVariant velocity = 0.1;            // 初始速度，值越小滑动越不灵敏
  properties.setScrollMetric(QScrollerProperties::DecelerationFactor,
                             decelerationFactor);
  properties.setScrollMetric(QScrollerProperties::MousePressEventDelay,
                             0.2); // 延迟处理鼠标按下事件
  properties.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor,
                             0.8); // 速度平滑因子
  properties.setScrollMetric(QScrollerProperties::MinimumVelocity,
                             0.0); // 最小速度
  properties.setScrollMetric(QScrollerProperties::MaximumVelocity,
                             0.5); // 最大速度
  properties.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor,
                             0.5); // 超出边界阻力
  properties.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor,
                             0.2); // 超出边界滚动距离
  scroller->setScrollerProperties(properties);

  // 设置窗口大小为 320x170
  resize(320, 170);

  // 加载文本文件
  if (!loadTextFile()) {
    textBrowser->setHtml("<p style='color: red;'>无法加载文本文件</p>");
  }

  // 恢复阅读进度
  restoreReadingProgress();
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

  QString text = QString::fromUtf8(textData);
  textBrowser->setPlainText(text);

  QString suffix = QFileInfo(currentPath).suffix().toLower();
  if (suffix == "json") {
    new JsonHighlighter(textBrowser->document());
  } else if (suffix == "cpp" || suffix == "h" || suffix == "c") {
    new CHighlighter(textBrowser->document());
  }

  return true;
}

void TextViewer::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  // 将关闭按钮放置在右上角
  if (closeButton) {
    closeButton->move(width() - closeButton->width() - 5, 5);
  }
}

// 获取进度文件路径
QString TextViewer::getProgressFilePath() {
  QFileInfo fileInfo(currentPath);
  QString dirPath = fileInfo.absolutePath();
  QString fileName = "." + fileInfo.completeBaseName() + "_progress.ini";
  return dirPath + "/" + fileName;
}

// 保存阅读进度
void TextViewer::saveReadingProgress() {
  QSettings settings(getProgressFilePath(), QSettings::IniFormat);
  settings.setValue("scrollPosition", textBrowser->verticalScrollBar()->value());
  settings.sync();
}

// 恢复阅读进度
void TextViewer::restoreReadingProgress() {
  QSettings settings(getProgressFilePath(), QSettings::IniFormat);
  int scrollPosition = settings.value("scrollPosition", 0).toInt();

  // 等待UI完全加载后再设置滚动位置
  QTimer::singleShot(100, [this, scrollPosition]() {
    textBrowser->verticalScrollBar()->setValue(scrollPosition);
  });
}

// 关闭事件处理，保存阅读进度
void TextViewer::closeEvent(QCloseEvent *event) {
  saveReadingProgress();
  QWidget::closeEvent(event);
}
