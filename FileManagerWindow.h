#ifndef FILEMANAGERWINDOW_H
#define FILEMANAGERWINDOW_H

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

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
  void updateBreadcrumbForItem(QWidget *breadcrumbBar);
  QString formatDisplayPath(const QString &path);
  void showDeleteConfirmationDialog(int itemIndex);
  void hideDeleteConfirmationDialog();

  // 左侧按钮栏
  QPushButton *btnBack;
  QPushButton *btnSort;
  QPushButton *btnEdit;
  QPushButton *btnDelete;

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
