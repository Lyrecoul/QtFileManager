#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QPointF>
#include <QTimer>

class QLabel;
class QPushButton;

class ImageViewer : public QWidget
{
    Q_OBJECT

public:
    explicit ImageViewer(const QString &path, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onZoomIn();
    void onZoomOut();
    void hideZoomButtons();

private:
    void updateImageDisplay();
    void adjustImagePosition();
    void rotateImage();
    void showZoomButtons();

    QPixmap originalPixmap;
    QPixmap rotatedPixmap;
    QString currentPath;

    double scaleFactor = 1.0;
    int rotationAngle = 0; // 0, 90, 180, 270

    QPointF lastMousePos;
    QPointF offset; // 当前图片偏移
    bool dragging = false;
    bool longPressDetected = false;
    QTimer longPressTimer;

    QPushButton *zoomInButton = nullptr;
    QPushButton *zoomOutButton = nullptr;
    QTimer zoomButtonHideTimer;
};

#endif // IMAGEVIEWER_H
