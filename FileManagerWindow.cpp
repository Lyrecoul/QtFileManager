// FileManagerWindow.cpp
#include "FileManagerWindow.h"
#include "FileItemDelegate.h"
#include "ImageViewer.h"
#include "MyListWidget.h"
#include "qscrollbar.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileIconProvider>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMimeDatabase>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QScroller>
#include <QScrollerProperties>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

FileManagerWindow::FileManagerWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
      sortMode(SortMode::Name) {

  setStyleSheet("background-color: #000000ff; color: white;");
  setWindowTitle("文件管理器");
  setAttribute(Qt::WA_DeleteOnClose, false);

  QFont font("Microsoft YaHei");
  this->setFont(font);

  setStyleSheet(R"(
  QWidget {
      background-color: #000000;  /* 纯黑 */
      color: #ffffff;
      font-size: 13px;
  }
  QPushButton {
      background-color: transparent;
      border: none;
      padding: 2px;
  }
  QPushButton:hover {
      background-color: #333333;
      border-radius: 6px;
  }
  QListWidget {
      background-color: transparent;
      border: none;
  }
)");

  // 左侧按钮栏
  QVBoxLayout *sideLayout = new QVBoxLayout();
  sideLayout->setSpacing(6);
  sideLayout->setContentsMargins(4, 4, 4, 4);

  auto createButton = [&](const QString &iconPath) -> QPushButton * {
    QPushButton *btn = new QPushButton();
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(20, 20));
    btn->setFixedSize(32, 32);
    btn->setStyleSheet(R"(
      QPushButton {
        background-color: #242424;
        border: none;
        padding: 2px;
        border-radius: 8px
      }
      QPushButton:hover {
        background-color: #3f3f3f;
        border-radius: 6px;
      }
    )");
    return btn;
  };

  QPushButton *btnBack = createButton(":/icons/back.png");
  connect(btnBack, &QPushButton::clicked, this, &FileManagerWindow::goBack);

  QPushButton *btnSort = createButton(":/icons/sort.png");
  connect(btnSort, &QPushButton::clicked, this, [this]() {
    switch (sortMode) {
    case SortMode::Name:
      sortMode = SortMode::Time;
      break;
    case SortMode::Time:
      sortMode = SortMode::Type;
      break;
    case SortMode::Type:
      sortMode = SortMode::Name;
      break;
    }
    loadFileItems(currentPath);
  });

  QPushButton *btnEdit = createButton(":/icons/edit.png");
  QPushButton *btnDelete = createButton(":/icons/delete.png");

  sideLayout->addWidget(btnBack);
  sideLayout->addWidget(btnSort);
  sideLayout->addWidget(btnEdit);
  sideLayout->addWidget(btnDelete);
  sideLayout->addStretch();

  QWidget *sideWidget = new QWidget();
  sideWidget->setLayout(sideLayout);
  sideWidget->setFixedWidth(40);

  // 面包屑路径导航栏（内嵌widget）
  breadcrumbBar = new QWidget();
  breadcrumbLayout = new QHBoxLayout(breadcrumbBar);
  breadcrumbLayout->setContentsMargins(4, 0, 4, 0);
  breadcrumbLayout->setSpacing(0);

  breadcrumbBar->setStyleSheet(R"(
    QToolButton {
        color: rgba(255, 255, 255, 0.6);
        background: transparent;
        border: none;
        padding: 4px 6px;
        font-weight: normal;
    }
    QToolButton:hover {
        background-color: #333333;
        border-radius: 6px;
    }
  )");

  // QScrollArea包裹breadcrumbBar，实现横向滑动
  QScrollArea *breadcrumbScroll = new QScrollArea();
  breadcrumbScroll->setWidget(breadcrumbBar);
  breadcrumbScroll->setWidgetResizable(true);
  breadcrumbScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  breadcrumbScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  breadcrumbScroll->setFixedHeight(32);
  breadcrumbScroll->setStyleSheet(
      "QScrollArea { background: transparent; border: none; }");

  // 支持触摸滑动：**绑定scrollArea的viewport**
  QScroller::grabGesture(breadcrumbScroll->viewport(), QScroller::TouchGesture);
  QScroller *scroller = QScroller::scroller(breadcrumbScroll->viewport());
  QScrollerProperties sp = scroller->scrollerProperties();
  sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                     QVariant::fromValue<QScrollerProperties::OvershootPolicy>(
                         QScrollerProperties::OvershootAlwaysOff));
  scroller->setScrollerProperties(sp);

  // 添加关闭按钮
  QToolButton *btnClose = new QToolButton(breadcrumbBar);
  btnClose->setIcon(QIcon(":/icons/close.png")); // 关闭图标路径
  btnClose->setIconSize(QSize(16, 16));
  btnClose->setFixedSize(28, 28);
  btnClose->setStyleSheet(R"(
  QToolButton {
    background-color: #242424;  /* 灰色背景 */
    border-radius: 14px;         /* 圆形 */
    border: none;
  }
  QToolButton:hover {
    background-color: #888888;
  }
)");
  btnClose->move(breadcrumbBar->width() - btnClose->width() - 4, 2);
  btnClose->raise();

  // 使用事件过滤器监听breadcrumbBar大小变化，确保关闭按钮始终在右上角
  breadcrumbBar->installEventFilter(this);
  btnClose->setProperty("isCloseButton", true); // 标记关闭按钮

  // 关闭按钮点击关闭窗口
  connect(btnClose, &QToolButton::clicked, this, &FileManagerWindow::close);

  // 文件列表
  fileList = new MyListWidget();
  fileList->setSpacing(4);
  fileList->setItemDelegate(new FileItemDelegate(this));
  fileList->setUniformItemSizes(true);
  fileList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  QScroller::grabGesture(fileList->viewport(), QScroller::TouchGesture);
  fileList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  fileList->setMaximumWidth(320 - 40 - 8);

  fileList->setStyleSheet(R"(
    QListWidget {
        background-color: transparent;
        border: none;
    }
    QScrollBar:vertical {
        width: 14px;
        background: transparent;
        margin: 3px 0 3px 0;
        border-radius: 4px;
    }
    QScrollBar::handle:vertical {
        background: #666666;
        min-height: 20px;
        border-radius: 4px;
    }
    QScrollBar::handle:vertical:hover {
        background: #888888;
    }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px;
        subcontrol-origin: margin;
    }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
        background: none;
    }
)");

  connect(fileList, &QListWidget::itemClicked, this,
          &FileManagerWindow::onItemClicked);

  // 主体区域布局
  QVBoxLayout *mainAreaLayout = new QVBoxLayout();
  mainAreaLayout->setContentsMargins(0, 4, 4, 4);
  mainAreaLayout->setSpacing(4);

  mainAreaLayout->addWidget(breadcrumbScroll);
  mainAreaLayout->addWidget(fileList);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(sideWidget);
  mainLayout->addLayout(mainAreaLayout);

  setLayout(mainLayout);

  // 初始路径加载
  currentPath = "/userdisk/Music";
  loadFileItems(currentPath);
}

void FileManagerWindow::loadFileItems(const QString &path) {
  QString rootPath = "/userdisk/Music";
  QString normalizedPath = QDir(path).absolutePath();
  if (!normalizedPath.startsWith(rootPath)) {
    currentPath = rootPath;
  } else {
    currentPath = normalizedPath;
  }

  fileList->clear();
  fileInfoList.clear();

  QDir dir(currentPath);
  QDir::SortFlags sortFlags = QDir::DirsFirst;
  switch (sortMode) {
  case SortMode::Name:
    sortFlags |= QDir::Name;
    break;
  case SortMode::Time:
    sortFlags |= QDir::Time;
    break;
  case SortMode::Type:
    sortFlags |= QDir::Type;
    break;
  }

  QFileInfoList entries =
      dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, sortFlags);
  QFileIconProvider iconProvider;

  if (entries.isEmpty()) {
    QListWidgetItem *item = new QListWidgetItem("此文件夹为空");
    item->setTextAlignment(Qt::AlignCenter);
    item->setForeground(QBrush(QColor("#888888")));
    item->setSizeHint(QSize(fileList->width(), 60));
    fileList->addItem(item);
  }

  for (const QFileInfo &info : entries) {
    QListWidgetItem *item = new QListWidgetItem();

    QFontMetrics fm(item->font());
    QString elided = fm.elidedText(info.fileName(), Qt::ElideRight, 150);
    item->setText(elided);

    item->setIcon(iconProvider.icon(info));
    item->setSizeHint(QSize(fileList->width(), 44)); // 视觉一致性
    fileList->addItem(item);
    fileInfoList.append(info);
  }

  updateBreadcrumb();
}

void FileManagerWindow::updateBreadcrumb() {
  // 清理旧面包屑
  QLayoutItem *child;
  while ((child = breadcrumbLayout->takeAt(0)) != nullptr) {
    if (child->widget()) {
      child->widget()->deleteLater();
    }
    delete child;
  }

  QString basePath = "/userdisk/Music";
  QString relativePath = currentPath.mid(basePath.length());
  QStringList parts = relativePath.split('/', Qt::SkipEmptyParts);
  QString pathAccumulator = basePath;

  auto addButton = [&](const QString &label, const QString &path,
                       bool isCurrent) {
    QToolButton *btn = new QToolButton();

    QFontMetrics fm(btn->font());
    QString elided = fm.elidedText(label, Qt::ElideRight, 150);
    btn->setText(elided);

    if (isCurrent) {
      // 当前路径高亮，纯白色加粗
      btn->setStyleSheet(R"(
        QToolButton {
          color: white;
          font-weight: bold;
          background: transparent;
          border: none;
          padding: 4px 6px;
        }
      )");
      btn->setEnabled(false);
    } else {
      // 非当前路径淡化
      btn->setStyleSheet(R"(
        QToolButton {
          color: rgba(255, 255, 255, 0.6);
          background: transparent;
          border: none;
          padding: 4px 6px;
        }
        QToolButton:hover {
          background-color: #333333;
          border-radius: 6px;
        }
      )");
      connect(btn, &QToolButton::clicked, this,
              [this, path]() { loadFileItems(path); });
    }

    breadcrumbLayout->addWidget(btn);
  };

  // 添加根目录按钮
  bool isCurrentRoot = (parts.isEmpty());
  addButton("存储", pathAccumulator, isCurrentRoot);

  for (int i = 0; i < parts.size(); ++i) {
    // 分隔符
    QLabel *sep = new QLabel(" > ");
    sep->setStyleSheet("color: rgba(255, 255, 255, 0.6);");
    breadcrumbLayout->addWidget(sep);

    pathAccumulator += "/" + parts[i];
    bool isCurrent = (i == parts.size() - 1);
    addButton(parts[i], pathAccumulator, isCurrent);
  }

  breadcrumbLayout->addStretch();

  // 计算面包屑总宽度，设置 breadcrumbBar 最小宽度，确保触发横向滚动
  int totalWidth = 0;
  for (int i = 0; i < breadcrumbLayout->count(); ++i) {
    if (auto w = breadcrumbLayout->itemAt(i)->widget()) {
      totalWidth += w->sizeHint().width();
    }
  }
  breadcrumbBar->setMinimumWidth(totalWidth + 20);

  // 自动滚动到最右侧，显示当前路径
  if (auto scrollArea =
          qobject_cast<QScrollArea *>(breadcrumbBar->parentWidget())) {
    QTimer::singleShot(0, [scrollArea]() {
      scrollArea->horizontalScrollBar()->setValue(
          scrollArea->horizontalScrollBar()->maximum());
    });
  }
}

void FileManagerWindow::goBack() {
  QDir dir(currentPath);
  if (dir.cdUp()) {
    QString newPath = dir.absolutePath();
    if (newPath.startsWith("/userdisk/Music")) {
      currentPath = newPath;
      loadFileItems(currentPath);
    }
  }
}

bool FileManagerWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == breadcrumbBar && event->type() == QEvent::Resize) {
    QResizeEvent *resizeEvent = static_cast<QResizeEvent *>(event);
    // 查找关闭按钮并移动它
    QToolButton *btnClose = breadcrumbBar->findChild<QToolButton *>(
        QString(), Qt::FindDirectChildrenOnly);
    if (btnClose && btnClose->property("isCloseButton").toBool()) {
      btnClose->move(resizeEvent->size().width() - btnClose->width() - 4, 2);
    }
  }
  return QWidget::eventFilter(watched, event);
}

void FileManagerWindow::onItemClicked(QListWidgetItem *item) {
  int row = fileList->row(item);
  if (row < 0 || row >= fileInfoList.size())
    return;

  QFileInfo info = fileInfoList[row];
  if (info.isDir()) {
    currentPath = info.absoluteFilePath();
    loadFileItems(currentPath);
    return;
  }

  QMimeDatabase db;
  QString mime = db.mimeTypeForFile(info).name();

  if (mime.startsWith("image/")) {
    auto *viewer = new ImageViewer(info.absoluteFilePath(), this);
    viewer->resize(320, 170);
    viewer->move(0, 0);
    viewer->show();
  } else if (mime.startsWith("video/") || mime.startsWith("audio/")) {
    QString program = QCoreApplication::applicationDirPath() + "/VideoPlayer";
    QStringList args{info.absoluteFilePath()};
    QProcess::startDetached(program, args);
  } else if (info.isExecutable()) {
    QProcess::startDetached(info.absoluteFilePath(), {});
  }
}
