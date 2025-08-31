#ifndef FILEMANAGERWINDOW_H
#define FILEMANAGERWINDOW_H

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMutex>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "MyListWidget.h"
#include "VirtualKeyboardWidget.h"

enum class SortMode { Name, Time, Size, Type };

class FileManagerWindow : public QWidget {
  Q_OBJECT

public:
  explicit FileManagerWindow(QWidget *parent = nullptr);

private slots:
  void goBack();
  void onItemClicked(QListWidgetItem *item);
  void startRename();
  void finishRename();
  void startDelete();
  void confirmDelete();
  void cancelDelete();
  void showSettingsMenu();
  void onSettingsLongPress();
  void directoryChanged(const QString &path);

private:
  void loadFileItems(const QString &path);
  void updateBreadcrumbForItem(QWidget *breadcrumbBar);
  QString formatDisplayPath(const QString &path);
  void showDeleteConfirmationDialog(int itemIndex);
  void hideDeleteConfirmationDialog();
  void showSettingsDialog();
  void loadSettings();
  void saveSettings();
  bool eventFilter(QObject *watched, QEvent *event) override;
  void setupDirectoryWatcher();

  // 左侧按钮栏
  QPushButton *btnBack;
  QPushButton *btnSettings;
  QPushButton *btnEdit;
  QPushButton *btnDelete;

  // 文件列表与路径数据
  MyListWidget *fileList;
  QList<QFileInfo> fileInfoList;
  QString currentPath;
  SortMode sortMode;
  
  // 隐藏设置
  bool hideMatchingLrcFiles;
  bool showHiddenFiles;
  QTimer *settingsLongPressTimer;
  
  // 排序设置
  bool reverseSortOrder;

  // 重命名功能
  bool isRenameMode;
  QLineEdit *renameEdit;
  VirtualKeyboardWidget *keyboard;
  int renameIndex;

  // 删除功能
  bool isDeleteMode;
  int deleteIndex;
  QWidget *deleteDialog;
  QPushButton *deleteConfirmButton;
  QPushButton *deleteCancelButton;
  
  // 分批加载控制
  bool loadingCancelled;
  QMutex loadingMutex;

  // 加载指示器
  QLabel *loadingIndicator;
  bool isLoading;

  // 文件夹监测器
  QFileSystemWatcher *dirWatcher;
  QTimer *dirChangeTimer;
};

#endif // FILEMANAGERWINDOW_H
