#include "FileManagerWindow.h"
#include "FileItemDelegate.h"
#include "ImageViewer.h"
#include "MarkdownViewer.h"
#include "MyListWidget.h"
#include "TextViewer.h"
#include "ToggleSwitch.h"
#include "VirtualKeyboardWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDir>
#include <QFileIconProvider>
#include <QFileSystemWatcher>
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
#include <QMutex>
#include <QMutexLocker>
#include <QEasingCurve>

FileManagerWindow::FileManagerWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint),
      sortMode(SortMode::Name), hideMatchingLrcFiles(false), showHiddenFiles(false),
      reverseSortOrder(false), isRenameMode(false), renameEdit(nullptr),
      keyboard(nullptr), renameIndex(-1), isDeleteMode(false), deleteIndex(-1),
      deleteDialog(nullptr), deleteConfirmButton(nullptr),
      deleteCancelButton(nullptr), loadingCancelled(false), loadingMutex(),
      loadingIndicator(nullptr), isLoading(false), dirWatcher(nullptr), dirChangeTimer(nullptr) {

  setWindowTitle("文件管理器");
  setAttribute(Qt::WA_DeleteOnClose, false);

  // 加载设置
  loadSettings();

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
  sideLayout->setSpacing(8);
  sideLayout->setContentsMargins(6, 6, 6, 6);

  auto createButton = [&](const QString &iconPath) {
    QPushButton *btn = new QPushButton();
    btn->setIcon(QIcon(iconPath));
    btn->setIconSize(QSize(20, 20));
    btn->setFixedSize(34, 34);
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

  btnSettings = createButton(":/icons/settings.png");
  connect(btnSettings, &QPushButton::clicked, this,
          &FileManagerWindow::showSettingsMenu);
  
  // 初始化长按计时器
  settingsLongPressTimer = new QTimer(this);
  settingsLongPressTimer->setSingleShot(true);
  settingsLongPressTimer->setInterval(3000); // 3秒
  connect(settingsLongPressTimer, &QTimer::timeout, this,
          &FileManagerWindow::onSettingsLongPress);
          
  // 设置按钮按下和释放事件
  btnSettings->installEventFilter(this);

  btnEdit = createButton(":/icons/edit.png");
  btnDelete = createButton(":/icons/delete.png");

  connect(btnEdit, &QPushButton::clicked, this,
          &FileManagerWindow::startRename);
  connect(btnDelete, &QPushButton::clicked, this,
          &FileManagerWindow::startDelete);

  sideLayout->addWidget(btnBack);
  sideLayout->addWidget(btnSettings);
  sideLayout->addWidget(btnEdit);
  sideLayout->addWidget(btnDelete);
  sideLayout->addStretch();

  QWidget *sideWidget = new QWidget();
  sideWidget->setLayout(sideLayout);
  sideWidget->setFixedWidth(48);

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

  // 创建加载指示器
  loadingIndicator = new QLabel("正在加载文件夹...");
  loadingIndicator->setAlignment(Qt::AlignCenter);
  loadingIndicator->setStyleSheet("background: transparent; color: #888888;");
  loadingIndicator->setFixedHeight(60);
  loadingIndicator->hide(); // 初始状态隐藏

  // 主体布局
  QVBoxLayout *mainAreaLayout = new QVBoxLayout();
  mainAreaLayout->setContentsMargins(0, 4, 4, 4);
  mainAreaLayout->setSpacing(4);
  mainAreaLayout->addWidget(fileList);
  mainAreaLayout->addWidget(loadingIndicator);

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

  // 初始化文件夹监测器
  setupDirectoryWatcher();

  loadFileItems(currentPath);
}

// 使用静态缓存存储图标，避免重复加载
static QCache<QString, QIcon> iconCache(100);

QIcon getMaterialIcon(const QFileInfo &info) {
  if (info.isDir()) {
    static QIcon folderIcon(":/icons/folder.png");
    return folderIcon;
  }

  // 检查文件是否可执行
  if (info.isExecutable()) {
    static QIcon exeIcon(":/icons/exe.png");
    return exeIcon;
  }

  QString suffix = info.suffix().toLower();
  
  // 检查缓存中是否已有该图标
  if (iconCache.contains(suffix)) {
    return *iconCache.object(suffix);
  }

  // 使用哈希表代替多个if-else语句，提高查找效率
  static const QHash<QString, QString> suffixToIconPath = {
    {"mp3", ":/icons/music.png"},
    {"flac", ":/icons/music.png"},
    {"wav", ":/icons/music.png"},
    {"aac", ":/icons/music.png"},
    {"ogg", ":/icons/music.png"},
    {"mp4", ":/icons/film.png"},
    {"mkv", ":/icons/film.png"},
    {"avi", ":/icons/film.png"},
    {"mov", ":/icons/film.png"},
    {"wmv", ":/icons/film.png"},
    {"png", ":/icons/image.png"},
    {"jpg", ":/icons/image.png"},
    {"jpeg", ":/icons/image.png"},
    {"bmp", ":/icons/image.png"},
    {"gif", ":/icons/image.png"},
    {"webp", ":/icons/image.png"},
    {"srt", ":/icons/subtitles.png"},
    {"ass", ":/icons/subtitles.png"},
    {"vtt", ":/icons/subtitles.png"},
    {"sub", ":/icons/subtitles.png"},
    {"lrc", ":/icons/lyrics.png"},
    {"md", ":/icons/markdown.png"},
    {"txt", ":/icons/text.png"},
    {"log", ":/icons/text.png"},
    {"ini", ":/icons/text.png"},
    {"conf", ":/icons/text.png"},
    {"json", ":/icons/json.png"},
    {"img", ":/icons/disk.png"},
    {"iso", ":/icons/disk.png"},
    {"wim", ":/icons/disk.png"},
    {"zip", ":/icons/zip.png"},
    {"rar", ":/icons/zip.png"},
    {"tar", ":/icons/zip.png"},
    {"gz", ":/icons/zip.png"},
    {"c", ":/icons/code.png"},
    {"cpp", ":/icons/code.png"},
    {"h", ":/icons/code.png"},
    {"py", ":/icons/code.png"},
    {"java", ":/icons/code.png"},
    {"js", ":/icons/code.png"},
    {"html", ":/icons/code.png"},
    {"css", ":/icons/code.png"},
    {"exe", ":/icons/exe.png"},
    {"sh", ":/icons/exe.png"},
    {"bat", ":/icons/exe.png"},
    {"msi", ":/icons/exe.png"},
    {"app", ":/icons/exe.png"},
    {"command", ":/icons/exe.png"},
    {"out", ":/icons/exe.png"},
    {"AppImage", ":/icons/exe.png"},
    {"bin", ":/icons/exe.png"},
    {"run", ":/icons/exe.png"}
  };

  // 查找对应的图标路径
  QString iconPath = suffixToIconPath.value(suffix, ":/icons/unknown.png");
  QIcon *icon = new QIcon(iconPath);
  
  // 将图标存入缓存
  iconCache.insert(suffix, icon);
  
  return *icon;
}

void FileManagerWindow::loadFileItems(const QString &path) {
  QMutexLocker locker(&loadingMutex);
  
  // 取消之前的加载操作
  loadingCancelled = true;

  // 保存当前滚动位置（如果路径相同）
  int scrollPosition = -1;
  if (path == currentPath && fileList) {
    scrollPosition = fileList->verticalScrollBar()->value();
  }
  
  QString musicPath = "/userdisk/Music";
  QString rootPath = QDir(musicPath).exists() ? musicPath : QDir::homePath();
  QString normalizedPath = QDir(path).absolutePath();
  currentPath = normalizedPath.startsWith(rootPath) ? normalizedPath : rootPath;

  // 更新文件夹监测器路径
  if (dirWatcher) {
    // 先移除旧的监测路径
    if (!dirWatcher->directories().isEmpty()) {
      dirWatcher->removePaths(dirWatcher->directories());
    }
    // 添加新的监测路径
    dirWatcher->addPath(currentPath);
  }

  // 先检查文件夹内的文件数量，只有在文件数量较多时才显示加载指示器
  QDir countDir(currentPath);
  QDir::Filters countFilters = QDir::AllEntries | QDir::NoDotAndDotDot;
  if (showHiddenFiles) {
    countFilters |= QDir::Hidden;
  }
  int fileCount = countDir.entryList(countFilters).size();

  // 只有当文件数量超过 50 个时才显示加载指示器
  if (fileCount > 50) {
    isLoading = true;
    fileList->hide();
    loadingIndicator->show();
    QCoreApplication::processEvents(); // 确保UI立即更新
  } else {
    isLoading = false;
  }

  // 完全清除旧内容，确保没有残留
  fileList->setUpdatesEnabled(false);
  fileList->clear();
  fileInfoList.clear();
  // 强制立即处理所有待处理事件，确保清除完成
  QCoreApplication::processEvents();
  
  // 重置取消标志，开始新的加载
  loadingCancelled = false;

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
  else if (sortMode == SortMode::Size)
    sortFlags |= QDir::Size;
  else if (sortMode == SortMode::Type)
    sortFlags |= QDir::Type;

  // 如果启用反转排序，添加 Reversed 标志
  if (reverseSortOrder) {
    sortFlags |= QDir::Reversed;
  }

  QDir::Filters filters = QDir::AllEntries | QDir::NoDotAndDotDot;
  if (showHiddenFiles) {
    filters |= QDir::Hidden;
  }
  QFileInfoList entries = dir.entryInfoList(filters, sortFlags);

  // 如果启用了隐藏与歌曲匹配的 lrc 文件功能，则过滤掉这些文件
  if (hideMatchingLrcFiles) {
    QFileInfoList filteredEntries;
    QSet<QString> musicFiles;

    // 使用更高效的方式收集音乐文件
    static const QSet<QString> musicExtensions = {"mp3", "flac", "wav", "aac", "ogg"};
    for (const QFileInfo &info : entries) {
      if (info.isFile() && musicExtensions.contains(info.suffix().toLower())) {
        musicFiles.insert(info.completeBaseName());
      }
    }

    // 过滤掉与音乐文件匹配的 lrc 文件
    for (const QFileInfo &info : entries) {
      if (info.isFile() && info.suffix().toLower() == "lrc") {
        if (musicFiles.contains(info.completeBaseName())) {
          continue; // 跳过与音乐文件匹配的 lrc 文件
        }
      }
      filteredEntries.append(info);
    }

    entries = filteredEntries;
  }

  if (entries.isEmpty()) {
    QLabel *emptyLabel = new QLabel("此文件夹为空");
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("background: transparent; color: #888888;");
    emptyLabel->setFixedHeight(60);

    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(fileList->width(), 60));
    fileList->addItem(item);
    fileList->setItemWidget(item, emptyLabel);
    fileList->setUpdatesEnabled(true);

    // 只在显示过加载指示器的情况下才隐藏它
    if (isLoading) {
      isLoading = false;
      loadingIndicator->hide();
      fileList->show();
    }
    return;
  }

  // 使用延迟加载处理大量文件，但根据文件数量动态调整批次大小
  int totalFiles = entries.size();
  // 文件数量越多，批次大小越大，减少UI更新次数
  int batchSize = qMin(100 + totalFiles / 50, 200);
  int loadedFiles = 0;
  
  // 预分配空间
  fileInfoList.reserve(totalFiles);
  
  // 批量处理文件
  while (loadedFiles < totalFiles && !loadingCancelled) {
    int endIndex = qMin(loadedFiles + batchSize, totalFiles);
    
    // 处理当前批次
    for (int i = loadedFiles; i < endIndex && !loadingCancelled; ++i) {
      const QFileInfo &info = entries[i];
      QListWidgetItem *item = new QListWidgetItem();
      QFontMetrics fm(item->font());
      item->setText(fm.elidedText(info.fileName(), Qt::ElideRight, 150));
      item->setIcon(getMaterialIcon(info));
      item->setSizeHint(QSize(fileList->width(), 44));
      fileList->addItem(item);
      fileInfoList.append(info);
    }
    
    // 更新已加载的文件数量
    loadedFiles = endIndex;
    
    // 每处理一批次，允许UI更新一次
    fileList->setUpdatesEnabled(true);
    // 只在文件数量较多时才处理事件，减少UI阻塞
    if (totalFiles > 500) {
      QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    fileList->setUpdatesEnabled(false);
    
    // 检查是否被取消
    if (loadingCancelled) {
      // 清理已加载但未完成的部分
      fileList->clear();
      fileInfoList.clear();

      // 只在显示过加载指示器的情况下才隐藏它
      if (isLoading) {
        isLoading = false;
        loadingIndicator->hide();
        fileList->show();
      }
      break;
    }
  }
  
  // 最后启用更新
  fileList->setUpdatesEnabled(true);

  // 恢复滚动位置（如果之前保存了）
  if (scrollPosition >= 0 && fileList->verticalScrollBar()) {
    fileList->verticalScrollBar()->setValue(scrollPosition);
  }

  // 只在显示过加载指示器的情况下才隐藏它
  if (isLoading) {
    isLoading = false;
    loadingIndicator->hide();
    fileList->show();
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
        
        // 重新启用删除按钮
        btnDelete->setEnabled(true);
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
      QProcess::startDetached("/userdisk/VideoPlayer", {info.absoluteFilePath()});
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

  // 重新启用重命名按钮
  btnEdit->setEnabled(true);
}

void FileManagerWindow::cancelDelete() {
  hideDeleteConfirmationDialog();
  // 保持在删除模式，用户可以选择其他文件
}

void FileManagerWindow::showSettingsMenu() { showSettingsDialog(); }

void FileManagerWindow::onSettingsLongPress() {
  // 切换显示/隐藏隐藏文件夹
  showHiddenFiles = !showHiddenFiles;
  saveSettings();
  loadFileItems(currentPath);
}

void FileManagerWindow::directoryChanged(const QString &path) {
  // 当监测的文件夹内容发生变化时，启动防抖动定时器
  // 这样可以避免在短时间内多次刷新列表
  if (path == currentPath && dirChangeTimer) {
    dirChangeTimer->start();
  }
}

void FileManagerWindow::showSettingsDialog() {
  QDialog dlg(this);
  dlg.setFixedSize(320, 170);
  dlg.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
  dlg.setStyleSheet("QDialog { background-color: #121212; border: none; "
                    "border-radius: 16px; }");

  QScrollArea *scrollArea = new QScrollArea(&dlg);
  scrollArea->setWidgetResizable(true);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setStyleSheet(R"(
  QScrollArea { background: transparent; border: none; }
  QScrollBar:vertical, QScrollBar:horizontal {
    width: 0px;
    height: 0px;
    background: transparent;
  }
  QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
    background: transparent;
    min-height: 0;
    min-width: 0;
  }
)");

  QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);

  QWidget *contentWidget = new QWidget;
  QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(8, 8, 8, 8);
  contentLayout->setSpacing(12);

  // 创建统一风格的标签
  auto createLabel = [](const QString &text, const QString &style) {
    QLabel *label = new QLabel(text);
    label->setStyleSheet(style + " border: none;");
    return label;
  };

  // 左上角圆形关闭按钮
  QPushButton *closeBtn = new QPushButton("<");
  closeBtn->setFixedSize(32, 32);
  closeBtn->setStyleSheet(
      "QPushButton { background-color: #2d2d2d; color: #aaaaaa; border: none; "
      "border-radius: 16px; font-size: 16px; }"
      "QPushButton:pressed { background-color: #444444; color: white; }");
  QHBoxLayout *topLayout = new QHBoxLayout;
  topLayout->addWidget(closeBtn);
  topLayout->addStretch();
  contentLayout->addLayout(topLayout);

  // 排序方式标题
  QLabel *sortTitle = createLabel(
      "选择排序方式", "color: #ffffff; font-size: 14px; font-weight: bold;");
  contentLayout->addWidget(sortTitle);

  // 排序方式卡片
  QWidget *sortCard = new QWidget;
  sortCard->setStyleSheet("background-color: #1e1e1e; border-radius: 12px; "
                          "border: 1px solid #2d2d2d;");
  QVBoxLayout *sortLayout = new QVBoxLayout(sortCard);
  sortLayout->setContentsMargins(12, 12, 12, 12);
  sortLayout->setSpacing(8);

  QPushButton *btnName = new QPushButton("文件名");
  QPushButton *btnDate = new QPushButton("修改日期");
  QPushButton *btnSize = new QPushButton("大小");
  QPushButton *btnType = new QPushButton("类型");

  QString btnStyle =
      "QPushButton { color: #ffffff; background: #3a3a3a; border: none; "
      "border-radius: 12px; font-size: 13px; padding: 6px 12px; }"
      "QPushButton:checked { background: #ff2d3c; }";

  btnName->setCheckable(true);
  btnDate->setCheckable(true);
  btnSize->setCheckable(true);
  btnType->setCheckable(true);

  btnName->setStyleSheet(btnStyle);
  btnDate->setStyleSheet(btnStyle);
  btnSize->setStyleSheet(btnStyle);
  btnType->setStyleSheet(btnStyle);

  QButtonGroup *sortGroup = new QButtonGroup(&dlg);
  sortGroup->setExclusive(true);
  sortGroup->addButton(btnName, 0);
  sortGroup->addButton(btnDate, 1);
  sortGroup->addButton(btnSize, 2);
  sortGroup->addButton(btnType, 3);

  switch (sortMode) {
  case SortMode::Name:
    btnName->setChecked(true);
    break;
  case SortMode::Time:
    btnDate->setChecked(true);
    break;
  case SortMode::Size:
    btnSize->setChecked(true);
    break;
  case SortMode::Type:
    btnType->setChecked(true);
    break;
  }

  QHBoxLayout *row1 = new QHBoxLayout;
  row1->addWidget(btnName);
  row1->addWidget(btnDate);
  QHBoxLayout *row2 = new QHBoxLayout;
  row2->addWidget(btnSize);
  row2->addWidget(btnType);
  sortLayout->addLayout(row1);
  sortLayout->addLayout(row2);
  contentLayout->addWidget(sortCard);

  // 其他设置标题
  QLabel *otherTitle = createLabel(
      "其他设置", "color: #ffffff; font-size: 14px; font-weight: bold;");
  contentLayout->addWidget(otherTitle);

  // 开关控件
  ToggleSwitch *reverseSwitch = new ToggleSwitch;
  reverseSwitch->setChecked(reverseSortOrder);

  ToggleSwitch *hideLrcSwitch = new ToggleSwitch;
  hideLrcSwitch->setChecked(hideMatchingLrcFiles);

  // 反转排列顺序卡片
  QWidget *reverseCard = new QWidget;
  reverseCard->setStyleSheet("background-color: #1e1e1e; border-radius: 12px; "
                             "border: 1px solid #2d2d2d;");
  QHBoxLayout *reverseLayout = new QHBoxLayout(reverseCard);
  reverseLayout->setContentsMargins(16, 12, 16, 12);
  QLabel *label1 =
      createLabel("反转排列顺序", "color: #ffffff; font-size: 14px;");
  reverseLayout->addWidget(label1);
  reverseLayout->addStretch();
  reverseLayout->addWidget(reverseSwitch);
  contentLayout->addWidget(reverseCard);

  // 自动隐藏歌词文件卡片
  QWidget *hideCard = new QWidget;
  hideCard->setStyleSheet("background-color: #1e1e1e; border-radius: 12px; "
                          "border: 1px solid #2d2d2d;");
  QVBoxLayout *hideLayout = new QVBoxLayout(hideCard);
  hideLayout->setContentsMargins(16, 12, 16, 12);
  QHBoxLayout *hideTopRow = new QHBoxLayout;
  QLabel *label2 =
      createLabel("自动隐藏歌词文件", "color: #ffffff; font-size: 14px;");
  hideTopRow->addWidget(label2);
  hideTopRow->addStretch();
  hideTopRow->addWidget(hideLrcSwitch);
  hideLayout->addLayout(hideTopRow);

  QLabel *desc2 = createLabel("隐藏已匹配到歌曲的 lrc 歌词文件。",
                              "color: #aaaaaa; font-size: 12px;");
  desc2->setWordWrap(true);
  hideLayout->addWidget(desc2);
  contentLayout->addWidget(hideCard);

  contentLayout->addStretch();
  scrollArea->setWidget(contentWidget);

  QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->addWidget(scrollArea);

  QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
  dlg.exec();

  int sortId = sortGroup->checkedId();
  switch (sortId) {
  case 0:
    sortMode = SortMode::Name;
    break;
  case 1:
    sortMode = SortMode::Time;
    break;
  case 2:
    sortMode = SortMode::Size;
    break;
  case 3:
    sortMode = SortMode::Type;
    break;
  }

  reverseSortOrder = reverseSwitch->isChecked();
  hideMatchingLrcFiles = hideLrcSwitch->isChecked();
  saveSettings();
  loadFileItems(currentPath);
}

void FileManagerWindow::loadSettings() {
  QSettings settings("FileManager", "Settings");

  // 加载排序模式
  int sortModeValue =
      settings.value("SortMode", static_cast<int>(SortMode::Name)).toInt();
  sortMode = static_cast<SortMode>(sortModeValue);

  // 加载反转排序设置
  reverseSortOrder = settings.value("ReverseSortOrder", false).toBool();

  // 加载隐藏歌词文件设置
  hideMatchingLrcFiles = settings.value("HideMatchingLrcFiles", false).toBool();
  
  // 加载显示隐藏文件设置
  showHiddenFiles = settings.value("ShowHiddenFiles", false).toBool();
}

void FileManagerWindow::saveSettings() {
  QSettings settings("FileManager", "Settings");

  // 保存排序模式
  settings.setValue("SortMode", static_cast<int>(sortMode));

  // 保存反转排序设置
  settings.setValue("ReverseSortOrder", reverseSortOrder);

  // 保存隐藏歌词文件设置
  settings.setValue("HideMatchingLrcFiles", hideMatchingLrcFiles);
  
  // 保存显示隐藏文件设置
  settings.setValue("ShowHiddenFiles", showHiddenFiles);
}

void FileManagerWindow::setupDirectoryWatcher() {
  // 创建文件夹监测器
  if (!dirWatcher) {
    dirWatcher = new QFileSystemWatcher(this);
    connect(dirWatcher, &QFileSystemWatcher::directoryChanged, 
            this, &FileManagerWindow::directoryChanged);
  }

  // 创建防抖动定时器
  if (!dirChangeTimer) {
    dirChangeTimer = new QTimer(this);
    dirChangeTimer->setSingleShot(true);
    dirChangeTimer->setInterval(500); // 500ms 防抖动
    connect(dirChangeTimer, &QTimer::timeout, [this]() {
      if (!currentPath.isEmpty()) {
        loadFileItems(currentPath);
      }
    });
  }

  // 监测当前路径
  if (!currentPath.isEmpty()) {
    dirWatcher->addPath(currentPath);
  }
}

bool FileManagerWindow::eventFilter(QObject *watched, QEvent *event) {
  // 处理设置按钮的长按事件
  if (watched == btnSettings) {
    if (event->type() == QEvent::MouseButtonPress) {
      settingsLongPressTimer->start();
    } else if (event->type() == QEvent::MouseButtonRelease) {
      settingsLongPressTimer->stop();
    }
  }
  
  // 处理设置对话框的点击外部关闭事件
  if (event->type() == QEvent::MouseButtonPress) {
    QWidget *dialog = qobject_cast<QWidget *>(watched);
    if (dialog && dialog->windowFlags() & Qt::Popup) {
      QPoint pos =
          dialog->mapFromGlobal(static_cast<QMouseEvent *>(event)->globalPos());
      if (!dialog->rect().contains(pos)) {
        dialog->close();
        return true;
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}
