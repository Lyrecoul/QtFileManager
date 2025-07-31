#ifndef TEXTVIEWER_H
#define TEXTVIEWER_H

#include <QWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QFile>

class TextViewer : public QWidget {
    Q_OBJECT

public:
    explicit TextViewer(const QString &path, QWidget *parent = nullptr);

private:
    QString buttonStyle() const;
    bool loadTextFile();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QString currentPath;
    QTextBrowser *textBrowser;
    QPushButton *closeButton;
};

#endif // TEXTVIEWER_H
