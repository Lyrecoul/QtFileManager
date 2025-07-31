#ifndef MARKDOWNVIEWER_H
#define MARKDOWNVIEWER_H

#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QTextBrowser>
#include <QByteArray>
#include <QFile>

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

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QString currentPath;
    QTextBrowser *textBrowser;
    QByteArray htmlOutput;

    QPushButton *closeButton;
};

#endif // MARKDOWNVIEWER_H
