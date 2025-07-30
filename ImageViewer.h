#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QPointF>
#include <QGestureEvent>
#include <QShowEvent>

class ImageViewer : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewer(const QString &path, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);

private slots:
    void onZoomIn();
    void onZoomOut();
    void showZoomButtons();
    void hideZoomButtons();

private:
    void updateImageDisplay();
    void rotateImage();
    QString buttonStyle() const;

private:
    QString currentPath;

    QPixmap originalPixmap;
    QPixmap rotatedPixmap;

    double scaleFactor = 1.0;
    int rotationAngle = 0;

    QPointF offset;
    QPoint lastMousePos;
    bool dragging = false;

    QPushButton *zoomInButton = nullptr;
    QPushButton *zoomOutButton = nullptr;
    QPushButton *rotateButton = nullptr;
    QPushButton *closeButton = nullptr;
    QTimer zoomButtonHideTimer;
};

#endif // IMAGEVIEWER_H
