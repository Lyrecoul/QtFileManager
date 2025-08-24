// ImageViewer.h
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QPointF>
#include <QGestureEvent>
#include <QShowEvent>
#include <QDir>
#include <QFileInfoList>
#include <QCache>
#include <QScrollArea>
#include <QListWidget>
#include <QFutureWatcher>
#include <QMutex>
#include <QPair>
#include <QList>
#include <QMovie>

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
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
    bool gestureEvent(QGestureEvent *event);
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onZoomIn();
    void onZoomOut();
    void showZoomButtons();
    void hideZoomButtons();
    void onThumbnailClicked(QListWidgetItem* item);
    void loadThumbnailsInBackground();
    void onThumbnailLoaded(const QString& path, const QPixmap& pixmap);
    void updateGifFrame();

private:
    void updateImageDisplay();
    void rotateImage();
    QString buttonStyle() const;
    void initThumbnailView();
    void scanImageFiles();
    QPixmap generateThumbnail(const QString& path);
    void showThumbnailMenu();
    void hideThumbnailMenu();
    void switchToImage(int index);
    void processBatchThumbnails(const QList<QPair<QString, QPixmap>>& batch);
    void setupGifAnimation();

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
    QPushButton *thumbnailButton = nullptr;
    QTimer zoomButtonHideTimer;

    // 缩略图相关
    QFileInfoList imageFiles;
    int currentImageIndex = -1;
    QListWidget* thumbnailList;
    QScrollArea* thumbnailScrollArea;
    QCache<QString, QPixmap> thumbnailCache;
    QFutureWatcher<void>* thumbnailWatcher;
    QMutex thumbnailMutex;
    bool thumbnailsVisible = false;

    // GIF动画相关
    bool isGif = false;
    QMovie* gifMovie = nullptr;
    QPixmap currentGifFrame;
};

#endif // IMAGEVIEWER_H
