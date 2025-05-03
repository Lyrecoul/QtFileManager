#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>

class QLabel;
class QPushButton;

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(const QString &path, QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void updateImageDisplay();
    void adjustWindowSize();

    QLabel *imageLabel;
    QPushButton *closeButton;
    QString currentPath;
    bool firstShow = true;
};

#endif // IMAGEVIEWER_H
