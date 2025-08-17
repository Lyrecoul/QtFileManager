#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H

#include <QWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QFile>
#include <QCloseEvent>  // 新增：关闭事件头文件
#include <QTimer>       // 新增：定时器头文件

class TextViewer : public QWidget {
    Q_OBJECT

public:
    explicit TextViewer(const QString &path, QWidget *parent = nullptr);

private:
    QString buttonStyle() const;
    bool loadTextFile();
    void saveReadingProgress(); // 新增：保存阅读进度
    void restoreReadingProgress(); // 新增：恢复阅读进度
    QString getProgressFilePath(); // 新增：获取进度文件路径

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override; // 新增：关闭事件处理

private:
    QString currentPath;
    QTextBrowser *textBrowser;
    QPushButton *closeButton;
};

#endif // TEXTVIEWER_H
