#ifndef MARKDOWNVIEWER_H
#define MARKDOWNVIEWER_H

#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QTextBrowser>
#include <QByteArray>
#include <QFile>
#include <QRegularExpression>  // 新增：正则表达式头文件

#include "MarkdownViewer/md4c/md4c.h"

class MarkdownViewer : public QWidget {
    Q_OBJECT

public:
    explicit MarkdownViewer(const QString &path, QWidget *parent = nullptr);

private:
    QString buttonStyle() const;
    bool loadMarkdownFile();
    void convertMarkdownToHtml(const QByteArray &markdown);
    static void htmlOutputCallback(const MD_CHAR *html, MD_SIZE size, void *userdata);
    void extractTableOfContents(const QByteArray &markdown);
    void showTableOfContents();
    void hideTableOfContents();
    void updateUIOnResize();  // 新增：延迟更新 UI 的函数

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QString currentPath;
    QString currentDirPath;  // 新增：缓存当前目录路径
    QTextBrowser *textBrowser;
    QByteArray htmlOutput;

    QPushButton *closeButton;
    QPushButton *tocButton;
    QWidget *tocPanel;
    QTextBrowser *tocBrowser;
    QStringList tocHeadings;
    bool tocVisible;
    QTimer resizeTimer;  // 新增：延迟处理 resize 事件的定时器

    // 新增：预编译的正则表达式
    QRegularExpression codeInlineRegex;
    QRegularExpression codeBlockRegex;
    QRegularExpression tableRegex;
    QRegularExpression thRegex;
    QRegularExpression tdRegex;
    QRegularExpression wikiImageRegex;
    QRegularExpression imgSrcRegex;
    QRegularExpression imgTagRegex;
};

#endif // MARKDOWNVIEWER_H
