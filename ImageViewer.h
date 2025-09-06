// ImageViewer.h
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QCache>
#include <QDir>
#include <QFileInfoList>
#include <QFutureWatcher>
#include <QGestureEvent>
#include <QList>
#include <QListWidget>
#include <QMovie>
#include <QMutex>
#include <QPair>
#include <QPixmap>
#include <QPointF>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>
#include <QWidget>

class ImageViewer : public QWidget {
  Q_OBJECT

public:
  explicit ImageViewer(const QString &path, QWidget *parent = nullptr);
  ~ImageViewer();

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
  void onThumbnailClicked(QListWidgetItem *item);
  void loadThumbnailsInBackground();
  void onThumbnailLoaded(const QString &path, const QPixmap &pixmap);
  void updateGifFrame();
  void updateWebpFrame();

private:
  void updateImageDisplay();
  void rotateImage();
  QString buttonStyle() const;
  void initThumbnailView();
  void scanImageFiles();
  QPixmap generateThumbnail(const QString &path);
  void showThumbnailMenu();
  void hideThumbnailMenu();
  void switchToImage(int index);
  void processBatchThumbnails(const QList<QPair<QString, QPixmap>> &batch);
  void setupGifAnimation();
  void loadWebpImage();
  void setupWebpAnimation(void *demuxPtr, const QByteArray &data);
  void loadWebpFrame(int frameIndex);

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
  QListWidget *thumbnailList;
  QScrollArea *thumbnailScrollArea;
  QCache<QString, QPixmap> thumbnailCache;
  QFutureWatcher<void> *thumbnailWatcher;
  QMutex thumbnailMutex;
  bool thumbnailsVisible = false;

  // GIF 动画相关
  bool isGif = false;
  QMovie *gifMovie = nullptr;
  QPixmap currentGifFrame;

  // WebP 相关
  bool isWebp = false;
  bool isAnimatedWebp = false;
  QByteArray webpData;
  QPixmap currentWebpFramePixmap;
  QTimer *webpAnimationTimer = nullptr;
  int currentWebpFrame = 0;
  int webpFrameCount = 0;
  int webpLoopCount = 0;
  int currentLoopCount = 0;
  uint32_t webpBackgroundColor = 0;
  int webpCanvasWidth = 0;
  int webpCanvasHeight = 0;
};

#endif // IMAGEVIEWER_H
