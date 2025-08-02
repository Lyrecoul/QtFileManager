#include "FileManagerWindow.h"
#include "FileItemDelegate.h"
#include "ImageViewer.h"
#include "MarkdownViewer.h"
#include "MyListWidget.h"
#include "TextViewer.h"
#include "VirtualKeyboardWidget.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileIconProvider>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QScrollerProperties>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

FileManagerWindow::FileManagerWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
      sortMode(SortMode::Name), isRenameMode(false), renameEdit(nullptr),
      keyboard(nullptr), renameIndex(-1), isDeleteMode(false), deleteIndex(-1),
      deleteDialog(nullptr), deleteConfirmButton(nullptr),
      deleteCancelButton(nullptr) {

  setWindowTitle("文件管理器");
  setAttribute(Qt::WA_DeleteOnClose, false);
  QFont font("Microsoft YaHei");
  setFont(font);

  setStyleSheet(R"(
    QWidget {
        background-color: #000000;
        color: #ffffff;
        font-family: "Microsoft YaHei";
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

  auto createButton = [&](const QString &iconPath) {
    QPushButton *btn = new QPushButton();
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(20, 20));
    btn->setFixedSize(32, 32);
    btn->setStyleSheet(R"(
      QPushButton {
        background-color: #242424;
        border: none;
        padding: 2px;
        border-radius: 8px;
      }
      QPushButton:hover {
        background-color: #3f3f3f;
        border-radius: 6px;
      }
    )");
    return btn;
  };

  btnBack = createButton(":/icons/back.png");
  connect(btnBack, &QPushButton::clicked, this, &FileManagerWindow::goBack);

  btnSort = createButton(":/icons/sort.png");
  connect(btnSort, &QPushButton::clicked, this, [this]() {
    sortMode = static_cast<SortMode>((static_cast<int>(sortMode) + 1) % 3);
    loadFileItems(currentPath);
  });

  btnEdit = createButton(":/icons/edit.png");
  btnDelete = createButton(":/icons/delete.png");

  connect(btnEdit, &QPushButton::clicked, this,
          &FileManagerWindow::startRename);
  connect(btnDelete, &QPushButton::clicked, this,
          &FileManagerWindow::startDelete);

  sideLayout->addWidget(btnBack);
  sideLayout->addWidget(btnSort);
  sideLayout->addWidget(btnEdit);
  sideLayout->addWidget(btnDelete);
  sideLayout->addStretch();

  QWidget *sideWidget = new QWidget();
  sideWidget->setLayout(sideLayout);
  sideWidget->setFixedWidth(40);

  // =============== 文件列表 ===============
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
    }
  )");

  connect(fileList, &QListWidget::itemClicked, this,
          &FileManagerWindow::onItemClicked);

  // 主体布局
  QVBoxLayout *mainAreaLayout = new QVBoxLayout();
  mainAreaLayout->setContentsMargins(0, 4, 4, 4);
  mainAreaLayout->setSpacing(4);
  mainAreaLayout->addWidget(fileList);

  QHBoxLayout *mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(sideWidget);
  mainLayout->addLayout(mainAreaLayout);

  setLayout(mainLayout);

  QString musicPath = "/userdisk/Music";
  if (QDir(musicPath).exists()) {
    currentPath = musicPath;
  } else {
    currentPath = QDir::homePath();
  }
  loadFileItems(currentPath);
}

QIcon getMaterialIcon(const QFileInfo &info) {
  if (info.isDir()) {
    return QIcon(":/icons/folder.png");
  }

  QString suffix = info.suffix().toLower();

  // 音频文件
  if (suffix == "mp3" || suffix == "flac" || suffix == "wav" ||
      suffix == "aac" || suffix == "ogg") {
    return QIcon(":/icons/music.png");
  }

  // 视频文件
  if (suffix == "mp4" || suffix == "mkv" || suffix == "avi" ||
      suffix == "mov" || suffix == "wmv") {
    return QIcon(":/icons/film.png");
  }

  // 图片文件
  if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" ||
      suffix == "bmp" || suffix == "gif" || suffix == "webp") {
    return QIcon(":/icons/image.png");
  }

  // 字幕文件
  if (suffix == "srt" || suffix == "ass" || suffix == "vtt" ||
      suffix == "sub") {
    return QIcon(":/icons/subtitles.png");
  }

  // 歌词文件
  if (suffix == "lrc") {
    return QIcon(":/icons/lyrics.png");
  }

  // Markdown 文件
  if (suffix == "md") {
    return QIcon(":/icons/markdown.png");
  }

  // 文本文件
  if (suffix == "txt" || suffix == "log" || suffix == "ini" ||
      suffix == "conf") {
    return QIcon(":/icons/text.png");
  }

  // JSON 文件
  if (suffix == "json") {
    return QIcon(":/icons/json.png");
  }

  // 磁盘镜像文件
  if (suffix == "img" || suffix == "iso" || suffix == "wim") {
    return QIcon(":/icons/disk.png");
  }

  // 压缩包文件
  if (suffix == "zip" || suffix == "rar" || suffix == "tar" || suffix == "gz") {
    return QIcon(":/icons/zip.png");
  }

  // 代码文件
  if (suffix == "c" || suffix == "cpp" || suffix == "h" || suffix == "py" ||
      suffix == "java" || suffix == "js" || suffix == "html" ||
      suffix == "css") {
    return QIcon(":/icons/code.png");
  }

  // 默认图标
  return QIcon(":/icons/unknown.png");
}

void FileManagerWindow::loadFileItems(const QString &path) {
  QString musicPath = "/userdisk/Music";
  QString rootPath = QDir(musicPath).exists() ? musicPath : QDir::homePath();
  QString normalizedPath = QDir(path).absolutePath();
  currentPath = normalizedPath.startsWith(rootPath) ? normalizedPath : rootPath;

  fileList->clear();
  fileInfoList.clear();

  // 添加面包屑导航作为第一项
  QListWidgetItem *breadcrumbItem = new QListWidgetItem();
  breadcrumbItem->setSizeHint(QSize(fileList->width(), 32));
  fileList->addItem(breadcrumbItem);

  // 创建面包屑导航容器
  QWidget *breadcrumbContainer = new QWidget();
  QHBoxLayout *breadcrumbContainerLayout = new QHBoxLayout(breadcrumbContainer);
  breadcrumbContainerLayout->setContentsMargins(4, 0, 4, 0);
  breadcrumbContainerLayout->setSpacing(0);

  // 创建面包屑导航栏
  QWidget *breadcrumbBar = new QWidget();
  QHBoxLayout *breadcrumbLayout = new QHBoxLayout(breadcrumbBar);
  breadcrumbLayout->setContentsMargins(0, 0, 0, 0);
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

  // 移除关闭按钮
  breadcrumbContainerLayout->addWidget(breadcrumbBar);

  // 将面包屑导航容器设置为列表项的部件
  fileList->setItemWidget(breadcrumbItem, breadcrumbContainer);

  // 更新面包屑导航内容
  updateBreadcrumbForItem(breadcrumbBar);

  QDir dir(currentPath);
  QDir::SortFlags sortFlags = QDir::DirsFirst;
  if (sortMode == SortMode::Name)
    sortFlags |= QDir::Name;
  else if (sortMode == SortMode::Time)
    sortFlags |= QDir::Time;
  else if (sortMode == SortMode::Type)
    sortFlags |= QDir::Type;

  QFileInfoList entries =
      dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, sortFlags);
  QFileIconProvider iconProvider;

  if (entries.isEmpty()) {
    QLabel *emptyLabel = new QLabel("此文件夹为空");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("background: transparent; color: #888888;");
    emptyLabel->setFixedHeight(60);

    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(fileList->width(), 60));
    fileList->addItem(item);
    fileList->setItemWidget(item, emptyLabel);
  }

  for (const QFileInfo &info : entries) {
    QListWidgetItem *item = new QListWidgetItem();
    QFontMetrics fm(item->font());
    item->setText(fm.elidedText(info.fileName(), Qt::ElideRight, 150));
    item->setIcon(getMaterialIcon(info));
    item->setSizeHint(QSize(fileList->width(), 44));
    fileList->addItem(item);
    fileInfoList.append(info);
  }
}

void FileManagerWindow::updateBreadcrumbForItem(QWidget *breadcrumbBar) {
  QHBoxLayout *breadcrumbLayout =
      qobject_cast<QHBoxLayout *>(breadcrumbBar->layout());
  if (!breadcrumbLayout)
    return;

  // 清除现有内容
  QLayoutItem *child;
  while ((child = breadcrumbLayout->takeAt(0))) {
    if (child->widget())
      child->widget()->deleteLater();
    delete child;
  }

  QString musicPath = "/userdisk/Music";
  QString basePath = QDir(musicPath).exists() ? musicPath : QDir::homePath();
  QStringList parts =
      currentPath.mid(basePath.length()).split('/', Qt::SkipEmptyParts);
  QString pathAccumulator = basePath;

  auto addButton = [&](const QString &label, const QString &path,
                       bool isCurrent) {
    QToolButton *btn = new QToolButton();
    QFontMetrics fm(btn->font());
    btn->setText(fm.elidedText(label, Qt::ElideRight, 150));
    if (isCurrent) {
      btn->setStyleSheet(
          "QToolButton { color: white; font-weight: bold; background: "
          "transparent; border: none; padding: 4px 6px; }");
      btn->setEnabled(false);
    } else {
      connect(btn, &QToolButton::clicked, this,
              [this, path]() { loadFileItems(path); });
    }
    breadcrumbLayout->addWidget(btn);
  };

  addButton("存储", pathAccumulator, parts.isEmpty());
  for (int i = 0; i < parts.size(); ++i) {
    breadcrumbLayout->addWidget(new QLabel(" > "));
    pathAccumulator += "/" + parts[i];
    addButton(parts[i], pathAccumulator, i == parts.size() - 1);
  }

  breadcrumbLayout->addStretch();
}

void FileManagerWindow::goBack() {
  QDir dir(currentPath);
  QString musicPath = "/userdisk/Music";
  QString rootPath = QDir(musicPath).exists() ? musicPath : QDir::homePath();

  // 检查是否已经在根目录
  if (currentPath == rootPath) {
    // 退无可退，关闭程序
    close();
    return;
  }

  if (dir.cdUp()) {
    QString newPath = dir.absolutePath();
    if (newPath.startsWith(rootPath)) {
      currentPath = newPath;
      loadFileItems(currentPath);
    }
  }
}

void FileManagerWindow::onItemClicked(QListWidgetItem *item) {
  int row = fileList->row(item);
  // 第一项是面包屑导航，不处理文件操作
  if (row == 0)
    return;

  // 调整行索引，因为第一项是面包屑导航
  int fileIndex = row - 1;
  if (fileIndex < 0 || fileIndex >= fileInfoList.size())
    return;

  if (isDeleteMode) {
    showDeleteConfirmationDialog(fileIndex);
    return;
  }

  if (isRenameMode) {
    // 重命名模式下，点击文件项开始重命名
    renameIndex = fileIndex;
    QFileInfo info = fileInfoList[fileIndex];

    // 创建编辑框
    renameEdit = new QLineEdit(this);
    renameEdit->setText(info.fileName());
    // 调整位置，考虑面包屑导航项的高度
    renameEdit->setGeometry(fileList->geometry().left() + 44,
                            fileList->geometry().top() + 32 + (fileIndex * 48) +
                                12,
                            fileList->width() - 72, 24);
    renameEdit->setStyleSheet(
        "background-color: #333333; color: white; border: 1px solid #555555; "
        "border-radius: 4px; padding: 2px;");
    renameEdit->show();
    renameEdit->setFocus();

    // 创建虚拟键盘，默认填充文件全名（包括后缀）
    keyboard =
        new VirtualKeyboardWidget(info.fileName(), "请输入新的文件名", this);
    keyboard->move(0, height() - keyboard->height());
    keyboard->show();

    // 连接信号
    connect(renameEdit, &QLineEdit::returnPressed, this,
            &FileManagerWindow::finishRename);
    connect(keyboard, &VirtualKeyboardWidget::textEntered, this,
            [this](const QString &text) {
              if (renameEdit) {
                renameEdit->setText(text);
                finishRename();
              }
            });

    // 连接虚拟键盘的取消信号，确保在关闭虚拟键盘时也清理输入框
    connect(keyboard, &VirtualKeyboardWidget::cancelled, this, [this]() {
      if (renameEdit && isRenameMode) {
        renameEdit->deleteLater();
        renameEdit = nullptr;
        keyboard->deleteLater();
        keyboard = nullptr;
        isRenameMode = false;
        renameIndex = -1;

        // 恢复列表项显示
        FileItemDelegate *delegate =
            qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
        if (delegate) {
          delegate->setShowEditIcon(false);
          fileList->update();
        }
      }
    });

    return;
  }

  QFileInfo info = fileInfoList[fileIndex];
  if (info.isDir()) {
    loadFileItems(info.absoluteFilePath());
  } else {
    QMimeDatabase db;
    QString mime = db.mimeTypeForFile(info).name();

    if (mime.startsWith("image/")) {
      auto *viewer = new ImageViewer(info.absoluteFilePath(), this);
      viewer->resize(320, 170);
      viewer->move(0, 0);
      viewer->show();
    } else if (mime.startsWith("video/") || mime.startsWith("audio/")) {
      QProcess::startDetached(QCoreApplication::applicationDirPath() +
                                  "/VideoPlayer",
                              {info.absoluteFilePath()});
    } else if (mime == "text/markdown" || info.suffix().toLower() == "md") {
      auto *viewer = new MarkdownViewer(info.absoluteFilePath(), this);
      viewer->resize(320, 170);
      viewer->move(0, 0);
      viewer->show();
    } else if (mime.startsWith("text/") || info.suffix().toLower() == "txt" ||
               info.suffix().toLower() == "log" ||
               info.suffix().toLower() == "ini" ||
               info.suffix().toLower() == "conf" ||
               info.suffix().toLower() == "json") {
      auto *viewer = new TextViewer(info.absoluteFilePath(), this);
      viewer->resize(320, 170);
      viewer->move(0, 0);
      viewer->show();
    } else if (info.isExecutable()) {
      QProcess::startDetached(info.absoluteFilePath(), {});
    }
  }
}

void FileManagerWindow::startRename() {
  // 如果当前处于删除模式，先退出删除模式
  if (isDeleteMode) {
    isDeleteMode = false;
    FileItemDelegate *delegate =
        qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
    if (delegate) {
      delegate->setShowDeleteIcon(false);
      fileList->update();
    }
    hideDeleteConfirmationDialog();
    btnDelete->setEnabled(true);
  }

  isRenameMode = !isRenameMode;

  // 更新列表项显示
  FileItemDelegate *delegate =
      qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
  if (delegate) {
    delegate->setShowEditIcon(isRenameMode);
    fileList->update();
  }

  // 根据重命名模式状态设置删除按钮的可用性
  btnDelete->setEnabled(!isRenameMode);

  // 如果取消重命名模式，清理相关资源
  if (!isRenameMode) {
    if (renameEdit) {
      renameEdit->deleteLater();
      renameEdit = nullptr;
    }
    if (keyboard) {
      keyboard->deleteLater();
      keyboard = nullptr;
    }
    renameIndex = -1;
  }
}

void FileManagerWindow::finishRename() {
  if (!isRenameMode || !renameEdit || renameIndex < 0 ||
      renameIndex >= fileInfoList.size())
    return;

  QString newName = renameEdit->text().trimmed();
  if (newName.isEmpty()) {
    // 如果新名称为空，取消重命名
    renameEdit->deleteLater();
    renameEdit = nullptr;
    keyboard->deleteLater();
    keyboard = nullptr;
    return;
  }

  QFileInfo oldInfo = fileInfoList[renameIndex];

  // 获取新路径
  QString newPath = oldInfo.absolutePath() + "/" + newName;

  // 重命名文件或文件夹
  if (QFile::rename(oldInfo.absoluteFilePath(), newPath)) {
    // 更新文件信息列表
    fileInfoList[renameIndex] = QFileInfo(newPath);
    // 更新列表项显示，注意第一项是面包屑导航，所以索引要加1
    fileList->item(renameIndex + 1)->setText(newName);
  }

  // 清理资源
  renameEdit->deleteLater();
  renameEdit = nullptr;
  keyboard->deleteLater();
  keyboard = nullptr;

  // 退出重命名模式
  isRenameMode = false;
  renameIndex = -1;

  // 更新列表项显示
  FileItemDelegate *delegate =
      qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
  if (delegate) {
    delegate->setShowEditIcon(false);
    fileList->update();
  }
}

void FileManagerWindow::startDelete() {
  // 如果当前处于重命名模式，先退出重命名模式
  if (isRenameMode) {
    isRenameMode = false;
    FileItemDelegate *delegate =
        qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
    if (delegate) {
      delegate->setShowEditIcon(false);
      fileList->update();
    }
    if (renameEdit) {
      renameEdit->deleteLater();
      renameEdit = nullptr;
    }
    if (keyboard) {
      keyboard->deleteLater();
      keyboard = nullptr;
    }
    renameIndex = -1;
    btnEdit->setEnabled(true);
  }

  isDeleteMode = !isDeleteMode;

  // 更新列表项显示
  FileItemDelegate *delegate =
      qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
  if (delegate) {
    delegate->setShowDeleteIcon(isDeleteMode);
    fileList->update();
  }

  // 根据删除模式状态设置编辑按钮的可用性
  btnEdit->setEnabled(!isDeleteMode);

  // 如果取消删除模式，清理相关资源
  if (!isDeleteMode) {
    hideDeleteConfirmationDialog();
  }
}

void FileManagerWindow::showDeleteConfirmationDialog(int itemIndex) {
  if (deleteDialog) {
    hideDeleteConfirmationDialog();
  }

  deleteIndex = itemIndex;
  QFileInfo info = fileInfoList[itemIndex];

  // 遮罩层
  deleteDialog = new QWidget(this, Qt::FramelessWindowHint);
  deleteDialog->setGeometry(0, 0, width(), height());
  deleteDialog->setStyleSheet("background-color: rgba(0, 0, 0, 180);");

  // 深色卡片容器
  QWidget *card = new QWidget(deleteDialog);
  card->setFixedSize(260, 160);
  card->move((width() - card->width()) / 2, (height() - card->height()) / 2);
  card->setStyleSheet(R"(
    background-color: #222;
    border-radius: 12px;
  )");

  QVBoxLayout *cardLayout = new QVBoxLayout(card);
  cardLayout->setContentsMargins(16, 16, 16, 16);
  cardLayout->setSpacing(16);

  // 主标题
  QLabel *title = new QLabel("确认删除？", card);
  title->setAlignment(Qt::AlignCenter);
  title->setStyleSheet("font-size: 18px; font-weight: bold; color: white;");
  cardLayout->addWidget(title);

  // 文件名（副标题）
  QLabel *subtitle = new QLabel(info.fileName(), card);
  subtitle->setAlignment(Qt::AlignCenter);
  subtitle->setStyleSheet("font-size: 14px; color: #bbbbbb;");
  cardLayout->addWidget(subtitle);

  // 按钮布局
  QHBoxLayout *btnLayout = new QHBoxLayout();
  btnLayout->setSpacing(12);

  // 取消按钮
  deleteCancelButton = new QPushButton("取消");
  deleteCancelButton->setStyleSheet(R"(
    QPushButton {
      background-color: #444;
      color: white;
      font-size: 14px;
      border-radius: 8px;
      padding: 10px;
    }
    QPushButton:hover {
      background-color: #666;
    }
  )");
  deleteCancelButton->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Preferred);
  connect(deleteCancelButton, &QPushButton::clicked, this,
          &FileManagerWindow::cancelDelete);
  btnLayout->addWidget(deleteCancelButton);

  // 删除按钮
  deleteConfirmButton = new QPushButton("删除");
  deleteConfirmButton->setStyleSheet(R"(
    QPushButton {
      background-color: #d32f2f;
      color: white;
      font-size: 14px;
      border-radius: 8px;
      padding: 10px;
    }
    QPushButton:hover {
      background-color: #e53935;
    }
  )");
  deleteConfirmButton->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Preferred);
  connect(deleteConfirmButton, &QPushButton::clicked, this,
          &FileManagerWindow::confirmDelete);
  btnLayout->addWidget(deleteConfirmButton);

  cardLayout->addLayout(btnLayout);

  deleteDialog->show();
}

void FileManagerWindow::hideDeleteConfirmationDialog() {
  if (deleteDialog) {
    deleteDialog->deleteLater();
    deleteDialog = nullptr;
    deleteConfirmButton = nullptr;
    deleteCancelButton = nullptr;
  }
}

void FileManagerWindow::confirmDelete() {
  if (deleteIndex < 0 || deleteIndex >= fileInfoList.size()) {
    hideDeleteConfirmationDialog();
    return;
  }

  QFileInfo info = fileInfoList[deleteIndex];
  bool success = false;

  if (info.isDir()) {
    QDir dir(info.absoluteFilePath());
    success = dir.removeRecursively();
  } else {
    success = QFile::remove(info.absoluteFilePath());
  }

  if (success) {
    // 从列表中移除该项，注意第一项是面包屑导航，所以索引要加1
    fileInfoList.removeAt(deleteIndex);
    delete fileList->takeItem(deleteIndex + 1);
  }

  // 关闭对话框并退出删除模式
  hideDeleteConfirmationDialog();
  isDeleteMode = false;

  // 更新列表项显示
  FileItemDelegate *delegate =
      qobject_cast<FileItemDelegate *>(fileList->itemDelegate());
  if (delegate) {
    delegate->setShowDeleteIcon(false);
    fileList->update();
  }
}

void FileManagerWindow::cancelDelete() {
  hideDeleteConfirmationDialog();
  // 保持在删除模式，用户可以选择其他文件
}
