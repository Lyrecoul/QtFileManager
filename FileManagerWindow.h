#ifndef FILEMANAGERWINDOW_H
#define FILEMANAGERWINDOW_H

#include <QWidget>
#include <QListWidget>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>

#include "MyListWidget.h"
#include "VirtualKeyboardWidget.h"

enum class SortMode { Name, Time, Type };

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

private:
  void loadFileItems(const QString &path);
  void updateBreadcrumb();
  QString formatDisplayPath(const QString &path);
  void showDeleteConfirmationDialog(int itemIndex);
  void hideDeleteConfirmationDialog();

  // 左侧按钮栏
  QPushButton *btnBack;
  QPushButton *btnSort;
  QPushButton *btnEdit;
  QPushButton *btnDelete;

  // 面包屑路径导航栏
  QWidget *breadcrumbBar;
  QHBoxLayout *breadcrumbLayout;
  QToolButton *btnClose;

  // 文件列表与路径数据
  MyListWidget *fileList;
  QList<QFileInfo> fileInfoList;
  QString currentPath;
  SortMode sortMode;

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
};

#endif // FILEMANAGERWINDOW_H
