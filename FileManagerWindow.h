#ifndef FILEMANAGERWINDOW_H
#define FILEMANAGERWINDOW_H

#include <QWidget>
#include <QFileSystemModel>
#include <QTreeView>
#include <QPushButton>

class FileManagerWindow : public QWidget {
    Q_OBJECT

public:
    explicit FileManagerWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void goBack();
    void onFileClicked(const QModelIndex &index);

private:
    QFileSystemModel *model;
    QTreeView *tree;
    QPushButton *backButton;

    const QString rootPath = "/userdisk/Music";  // 可根据实际需求修改根路径
};

#endif // FILEMANAGERWINDOW_H
