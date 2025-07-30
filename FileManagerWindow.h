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

#include "MyListWidget.h"

enum class SortMode { Name, Time, Type };

class FileManagerWindow : public QWidget {
  Q_OBJECT

public:
  explicit FileManagerWindow(QWidget *parent = nullptr);

private slots:
  void goBack();
  void onItemClicked(QListWidgetItem *item);

private:
  void loadFileItems(const QString &path);
  void updateBreadcrumb();
  QString formatDisplayPath(const QString &path);

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
};

#endif // FILEMANAGERWINDOW_H
