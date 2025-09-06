// ImageViewer.cpp
#include "ImageViewer.h"
#include "qnamespace.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPinchGesture>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QScroller>
#include <QThread>
#include <QTimer>
#include <QTransform>
#include <QtConcurrent>

// WebP 支持
#include <webp/decode.h>
#include <webp/demux.h>

ImageViewer::ImageViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path) {
  setWindowTitle(QFileInfo(path).fileName());
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_DeleteOnClose);

  // 检查是否是 GIF 文件
  isGif = path.toLower().endsWith(".gif");

  // 检查是否是 WebP 文件
  isWebp = path.toLower().endsWith(".webp");

  if (isGif) {
    // 初始化 GIF 动画
    setupGifAnimation();
  } else if (isWebp) {
    // 加载 WebP 图片
    loadWebpImage();
  } else {
    // 加载普通图片
    originalPixmap = QPixmap(currentPath);
    rotatedPixmap = originalPixmap;
  }

  if (originalPixmap.isNull() && !isGif) {
    QTimer::singleShot(1500, this, &ImageViewer::close);
  }

  // 缩放按钮
  zoomInButton = new QPushButton("＋", this);
  zoomOutButton = new QPushButton("－", this);
  zoomInButton->setVisible(false);
  zoomOutButton->setVisible(false);

  zoomInButton->setStyleSheet(buttonStyle());
  zoomOutButton->setStyleSheet(buttonStyle());

  zoomInButton->setFixedSize(40, 32);
  zoomOutButton->setFixedSize(40, 32);

  connect(zoomInButton, &QPushButton::clicked, this, &ImageViewer::onZoomIn);
  connect(zoomOutButton, &QPushButton::clicked, this, &ImageViewer::onZoomOut);
  connect(&zoomButtonHideTimer, &QTimer::timeout, this,
          &ImageViewer::hideZoomButtons);

  rotateButton = new QPushButton("↻", this);
  rotateButton->setVisible(false);
  rotateButton->setStyleSheet(buttonStyle());
  rotateButton->setFixedSize(40, 32);

  connect(rotateButton, &QPushButton::clicked, this, [this]() {
    rotateImage();
    updateImageDisplay();
    showZoomButtons(); // 重新计时按钮隐藏
  });

  // 缩略图按钮
  thumbnailButton = new QPushButton("▣", this);
  thumbnailButton->setVisible(false);
  thumbnailButton->setStyleSheet(buttonStyle());
  thumbnailButton->setFixedSize(40, 32);
  connect(thumbnailButton, &QPushButton::clicked, this, [this]() {
    if (thumbnailsVisible) {
      hideThumbnailMenu();
    } else {
      showThumbnailMenu();
    }
  });

  closeButton = new QPushButton("✕", this);
  closeButton->setVisible(false);
  closeButton->setStyleSheet(buttonStyle());
  closeButton->setFixedSize(40, 32);
  connect(closeButton, &QPushButton::clicked, this, &ImageViewer::close);

  // 初始化缩略图相关
  scanImageFiles();
  initThumbnailView();

  // 启动后台加载缩略图
  thumbnailWatcher = new QFutureWatcher<void>(this);
  connect(thumbnailWatcher, &QFutureWatcher<void>::finished,
          []() { qDebug() << "Thumbnail loading completed"; });

  loadThumbnailsInBackground();

  grabGesture(Qt::PinchGesture);
}

ImageViewer::~ImageViewer() {
  // 停止 GIF 动画（如果有）
  if (gifMovie) {
    gifMovie->stop();
    delete gifMovie;
  }

  // 停止 WebP 动画（如果有）
  if (webpAnimationTimer) {
    webpAnimationTimer->stop();
    delete webpAnimationTimer;
  }
}

QString ImageViewer::buttonStyle() const {
  return R"(
        QPushButton {
            color: white;
            font-size: 22px;
            background: rgba(0,0,0,120);
            border-radius: 16px;
            padding: 4px 12px;
        }
        QPushButton:hover {
            background: rgba(0,0,0,180);
        }
    )";
}

void ImageViewer::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);
  showFullScreen();
  updateImageDisplay(); // 会自动调整 scaleFactor
}

void ImageViewer::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  updateImageDisplay();

  // 调整缩略图区域大小（如果可见）
  if (thumbnailsVisible) {
    int menuHeight = 120;
    thumbnailScrollArea->setGeometry(0, height() - menuHeight, width(),
                                     menuHeight);
  }
}

void ImageViewer::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

  if (isGif) {
    if (currentGifFrame.isNull()) {
      painter.fillRect(rect(), Qt::black);
      painter.setPen(Qt::white);
      painter.drawText(rect(), Qt::AlignCenter, "无法加载 GIF");
      return;
    }

    // 使用GIF当前帧
    QSizeF scaledSize = currentGifFrame.size() * scaleFactor;
    QSizeF viewSize(width(), height());
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF topLeft =
        center - QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) +
        offset;

    QSizeF imgSize = scaledSize;
    QPointF minOffset(qMin(0.0, (viewSize.width() - imgSize.width()) / 2.0),
                      qMin(0.0, (viewSize.height() - imgSize.height()) / 2.0));
    QPointF maxOffset(qMax(0.0, (imgSize.width() - viewSize.width()) / 2.0),
                      qMax(0.0, (imgSize.height() - viewSize.height()) / 2.0));
    offset.setX(qBound(minOffset.x(), offset.x(), maxOffset.x()));
    offset.setY(qBound(minOffset.y(), offset.y(), maxOffset.y()));

    topLeft = center -
              QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) +
              offset;

    painter.fillRect(rect(), Qt::black);
    painter.drawPixmap(
        QRectF(topLeft, scaledSize), currentGifFrame,
        QRectF(0, 0, currentGifFrame.width(), currentGifFrame.height()));
  } else if (isWebp && isAnimatedWebp) {
    if (currentWebpFramePixmap.isNull()) {
      painter.fillRect(rect(), Qt::black);
      painter.setPen(Qt::white);
      painter.drawText(rect(), Qt::AlignCenter, "无法加载 WebP 动画");
      return;
    }

    // 使用 WebP 当前帧
    QSizeF scaledSize = currentWebpFramePixmap.size() * scaleFactor;
    QSizeF viewSize(width(), height());
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF topLeft =
        center - QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) +
        offset;

    QSizeF imgSize = scaledSize;
    QPointF minOffset(qMin(0.0, (viewSize.width() - imgSize.width()) / 2.0),
                      qMin(0.0, (viewSize.height() - imgSize.height()) / 2.0));
    QPointF maxOffset(qMax(0.0, (imgSize.width() - viewSize.width()) / 2.0),
                      qMax(0.0, (imgSize.height() - viewSize.height()) / 2.0));
    offset.setX(qBound(minOffset.x(), offset.x(), maxOffset.x()));
    offset.setY(qBound(minOffset.y(), offset.y(), maxOffset.y()));

    topLeft = center -
              QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) +
              offset;

    painter.fillRect(rect(), Qt::black);
    painter.drawPixmap(QRectF(topLeft, scaledSize), currentWebpFramePixmap,
                       QRectF(0, 0, currentWebpFramePixmap.width(),
                              currentWebpFramePixmap.height()));
  } else {
    if (rotatedPixmap.isNull()) {
      painter.fillRect(rect(), Qt::black);
      painter.setPen(Qt::white);
      painter.drawText(rect(), Qt::AlignCenter, "无法加载图片");
      return;
    }

    QSizeF scaledSize = rotatedPixmap.size() * scaleFactor;
    QSizeF viewSize(width(), height());
    QPointF center(width() / 2.0, height() / 2.0);
    QPointF topLeft =
        center - QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) +
        offset;

    QSizeF imgSize = scaledSize;
    QPointF minOffset(qMin(0.0, (viewSize.width() - imgSize.width()) / 2.0),
                      qMin(0.0, (viewSize.height() - imgSize.height()) / 2.0));
    QPointF maxOffset(qMax(0.0, (imgSize.width() - viewSize.width()) / 2.0),
                      qMax(0.0, (imgSize.height() - viewSize.height()) / 2.0));
    offset.setX(qBound(minOffset.x(), offset.x(), maxOffset.x()));
    offset.setY(qBound(minOffset.y(), offset.y(), maxOffset.y()));

    topLeft = center -
              QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) +
              offset;

    painter.fillRect(rect(), Qt::black);
    painter.drawPixmap(
        QRectF(topLeft, scaledSize), rotatedPixmap,
        QRectF(0, 0, rotatedPixmap.width(), rotatedPixmap.height()));
  }

  int margin = 12;
  int totalWidth = zoomInButton->width() + zoomOutButton->width() +
                   rotateButton->width() + thumbnailButton->width() +
                   closeButton->width() + 4 * margin;

  int baseX = (width() - totalWidth) / 2;

  zoomInButton->move(baseX, margin);
  zoomOutButton->move(baseX + zoomInButton->width() + margin, margin);
  rotateButton->move(baseX + zoomInButton->width() + zoomOutButton->width() +
                         2 * margin,
                     margin);
  thumbnailButton->move(baseX + zoomInButton->width() + zoomOutButton->width() +
                            rotateButton->width() + 3 * margin,
                        margin);
  closeButton->move(baseX + zoomInButton->width() + zoomOutButton->width() +
                        rotateButton->width() + thumbnailButton->width() +
                        4 * margin,
                    margin);
}

void ImageViewer::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    lastMousePos = event->pos();
    dragging = true;
    showZoomButtons();
  }
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event) {
  if (dragging) {
    QPointF delta = event->pos() - lastMousePos;
    offset += delta;
    lastMousePos = event->pos();
    update();
  }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  if (dragging) {
    dragging = false;
    showZoomButtons();
  }
}

void ImageViewer::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event);
  // 双击切换缩略图菜单显示状态
  if (thumbnailsVisible) {
    hideThumbnailMenu();
  } else {
    showThumbnailMenu();
  }
}

bool ImageViewer::event(QEvent *event) {
  if (event->type() == QEvent::Gesture)
    return gestureEvent(static_cast<QGestureEvent *>(event));
  return QWidget::event(event);
}

bool ImageViewer::gestureEvent(QGestureEvent *event) {
  if (QGesture *gesture = event->gesture(Qt::PinchGesture)) {
    QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
    scaleFactor *= pinch->scaleFactor();
    scaleFactor = qBound(0.05, scaleFactor, 5.0);
    update();
    showZoomButtons();
    return true;
  }
  return false;
}

void ImageViewer::onZoomIn() {
  scaleFactor *= 1.25;
  if (scaleFactor > 5.0)
    scaleFactor = 5.0;
  update();
  showZoomButtons();
}

void ImageViewer::onZoomOut() {
  double minScale = 0.2;
  if (isGif) {
    if (!currentGifFrame.isNull()) {
      double scaleW = double(width()) / currentGifFrame.width();
      double scaleH = double(height()) / currentGifFrame.height();
      minScale = qMin(scaleW, scaleH);
      minScale = qMin(minScale, 1.0);
      minScale = qMax(minScale, 0.05);
    }
  } else if (isWebp && isAnimatedWebp) {
    if (!currentWebpFramePixmap.isNull()) {
      double scaleW = double(width()) / currentWebpFramePixmap.width();
      double scaleH = double(height()) / currentWebpFramePixmap.height();
      minScale = qMin(scaleW, scaleH);
      minScale = qMin(minScale, 1.0);
      minScale = qMax(minScale, 0.05);
    }
  } else if (!rotatedPixmap.isNull()) {
    double scaleW = double(width()) / rotatedPixmap.width();
    double scaleH = double(height()) / rotatedPixmap.height();
    minScale = qMin(scaleW, scaleH);
    minScale = qMin(minScale, 1.0);
    minScale = qMax(minScale, 0.05);
  }
  scaleFactor /= 1.25;
  if (scaleFactor < minScale)
    scaleFactor = minScale;
  update();
  showZoomButtons();
}

void ImageViewer::showZoomButtons() {
  zoomInButton->setVisible(true);
  zoomOutButton->setVisible(true);
  rotateButton->setVisible(true);
  thumbnailButton->setVisible(true);
  closeButton->setVisible(true);
  zoomButtonHideTimer.start(1800);
}

void ImageViewer::hideZoomButtons() {
  zoomInButton->setVisible(false);
  zoomOutButton->setVisible(false);
  rotateButton->setVisible(false);
  thumbnailButton->setVisible(false);
  closeButton->setVisible(false);
}

void ImageViewer::updateImageDisplay() {
  if (isGif) {
    if (!currentGifFrame.isNull()) {
      double scaleW = double(width()) / currentGifFrame.width();
      double scaleH = double(height()) / currentGifFrame.height();
      double fitScale = qMin(scaleW, scaleH);
      fitScale = qMin(fitScale, 1.0);
      fitScale = qMax(fitScale, 0.05);
      scaleFactor = fitScale;
    }
  } else if (isWebp && isAnimatedWebp) {
    if (!currentWebpFramePixmap.isNull()) {
      double scaleW = double(width()) / currentWebpFramePixmap.width();
      double scaleH = double(height()) / currentWebpFramePixmap.height();
      double fitScale = qMin(scaleW, scaleH);
      fitScale = qMin(fitScale, 1.0);
      fitScale = qMax(fitScale, 0.05);
      scaleFactor = fitScale;
    }
  } else if (!rotatedPixmap.isNull()) {
    double scaleW = double(width()) / rotatedPixmap.width();
    double scaleH = double(height()) / rotatedPixmap.height();
    double fitScale = qMin(scaleW, scaleH);
    fitScale = qMin(fitScale, 1.0);
    fitScale = qMax(fitScale, 0.05);
    scaleFactor = fitScale;
  }
  update();
}

void ImageViewer::rotateImage() {
  rotationAngle = (rotationAngle + 90) % 360;
  QTransform trans;
  trans.rotate(rotationAngle);

  if (isGif) {
    // GIF 不支持旋转
    return;
  } else {
    rotatedPixmap = originalPixmap.transformed(trans, Qt::SmoothTransformation);
  }
  offset = QPointF(0, 0);
}

// 缩略图相关实现
void ImageViewer::scanImageFiles() {
  QFileInfo currentFile(currentPath);
  QDir dir(currentFile.dir());

  // 支持的图片格式
  QStringList filters;
  filters << "*.jpg" << "*.jpeg" << "*.png" << "*.bmp" << "*.gif" << "*.webp"
          << "*.JPG" << "*.JPEG" << "*.PNG" << "*.BMP" << "*.GIF" << "*.WEBP";

  imageFiles = dir.entryInfoList(filters, QDir::Files, QDir::Name);

  // 找到当前图片索引
  for (int i = 0; i < imageFiles.size(); ++i) {
    if (imageFiles[i].absoluteFilePath() == currentFile.absoluteFilePath()) {
      currentImageIndex = i;
      break;
    }
  }
}

void ImageViewer::initThumbnailView() {
  // 创建滚动区域
  thumbnailScrollArea = new QScrollArea(this);
  thumbnailScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  thumbnailScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  thumbnailScrollArea->setStyleSheet("background-color: rgba(0,0,0,200);");

  // 创建列表控件
  thumbnailList = new QListWidget(this);
  thumbnailList->setViewMode(QListWidget::IconMode);
  thumbnailList->setIconSize(QSize(120, 90));
  thumbnailList->setResizeMode(QListWidget::Adjust);
  thumbnailList->setGridSize(QSize(130, 100));
  thumbnailList->setSpacing(10);

  // 优化缩略图列表和滚动条样式，与主窗口风格一致
  thumbnailList->setStyleSheet(
      "QListWidget { background-color: transparent; border: none; }"
      "QListWidget::item { border: 2px solid transparent; border-radius: 4px; }"
      "QListWidget::item:selected { border: 2px solid #4CAF50; }"
      "QScrollBar:vertical { width: 22px; background: transparent; margin: 3px "
      "0 3px 0; border-radius: 4px; }"
      "QScrollBar::handle:vertical { background: #666666; min-height: 20px; "
      "border-radius: 4px; }"
      "QScrollBar::handle:vertical:hover { background: #888888; }"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: "
      "0px; }"
      "QScrollBar:horizontal { height: 0px; }");

  // 设置滚动属性
  thumbnailList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  thumbnailList->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

  // 启用触摸滚动，与主窗口保持一致
  QScroller::grabGesture(thumbnailList->viewport(), QScroller::TouchGesture);

  // 确保滚动条可以正常工作
  thumbnailScrollArea->setWidget(thumbnailList);
  thumbnailScrollArea->setWidgetResizable(true);

  // 禁用拖动功能但保持滚动功能
  thumbnailList->setDragEnabled(false);
  thumbnailList->setMovement(QListWidget::Static);

  connect(thumbnailList, &QListWidget::itemClicked, this,
          &ImageViewer::onThumbnailClicked);

  // 默认隐藏
  thumbnailScrollArea->setVisible(false);
}

QPixmap ImageViewer::generateThumbnail(const QString &path) {
  // 特殊处理 GIF 文件
  if (path.toLower().endsWith(".gif")) {
    QMovie movie(path);
    if (movie.isValid()) {
      // 获取 GIF 的第一帧作为缩略图
      QPixmap frame = movie.currentPixmap();
      if (!frame.isNull()) {
        return frame.scaled(120, 90, Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
      }
    }
  }

  // 处理 WebP 文件
  if (path.toLower().endsWith(".webp")) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      return QPixmap();
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
      return QPixmap();
    }

    // 检查是否是动画 WebP
    WebPData webp_data;
    webp_data.bytes = reinterpret_cast<const uint8_t *>(data.constData());
    webp_data.size = data.size();

    WebPDemuxer *demux = WebPDemux(&webp_data);
    QPixmap result;

    if (demux) {
      // 动画 WebP，获取第一帧
      WebPIterator iter;
      if (WebPDemuxGetFrame(demux, 1, &iter)) {
        int width, height;
        uint8_t *decoded_data = WebPDecodeRGBA(
            iter.fragment.bytes, iter.fragment.size, &width, &height);

        if (decoded_data) {
          QImage image(decoded_data, width, height, QImage::Format_RGBA8888);
          result = QPixmap::fromImage(image.copy());
          WebPFree(decoded_data);
        }
      }
      WebPDemuxReleaseIterator(&iter);
      WebPDemuxDelete(demux);
    } else {
      // 静态 WebP
      int width, height;
      uint8_t *decoded_data =
          WebPDecodeRGBA(reinterpret_cast<const uint8_t *>(data.constData()),
                         data.size(), &width, &height);

      if (decoded_data) {
        QImage image(decoded_data, width, height, QImage::Format_RGBA8888);
        result = QPixmap::fromImage(image.copy());
        WebPFree(decoded_data);
      }
    }

    if (!result.isNull()) {
      return result.scaled(120, 90, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
    }
  }

  // 处理普通图片
  QPixmap pixmap(path);
  if (pixmap.isNull())
    return QPixmap();

  return pixmap.scaled(120, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ImageViewer::loadThumbnailsInBackground() {
  thumbnailList->clear();

  // 获取当前图片目录，并在该目录下创建缩略图缓存文件夹
  QFileInfo currentFileInfo(currentPath);
  QString currentDirPath = currentFileInfo.absolutePath();
  QString cacheDirPath = currentDirPath + "/.thumbnails";
  QDir cacheDir(cacheDirPath);
  if (!cacheDir.exists()) {
    cacheDir.mkpath(".");
  }

  // 先添加占位项
  for (const auto &file : imageFiles) {
    auto item = new QListWidgetItem(QIcon(), "");
    item->setData(Qt::UserRole, file.absoluteFilePath());
    thumbnailList->addItem(item);

    // 标记当前图片
    if (file.absoluteFilePath() == currentPath) {
      thumbnailList->setCurrentItem(item);
    }
  }

  // 后台线程生成缩略图
  auto future = QtConcurrent::run([this, cacheDirPath]() {
    // 优化性能：批量处理缩略图
    QList<QPair<QString, QPixmap>> batchResults;
    const int batchSize = 10; // 每批处理 10 个

    for (int i = 0; i < imageFiles.size(); ++i) {
      const QString path = imageFiles[i].absoluteFilePath();

      // 生成缓存文件名（使用文件名 +
      // 文件大小作为哈希基础，确保文件修改后缓存失效）
      QFileInfo fileInfo(path);
      QString hashBase = fileInfo.fileName() + QString::number(fileInfo.size());
      QString fileHash = QString(
          QCryptographicHash::hash(hashBase.toUtf8(), QCryptographicHash::Md5)
              .toHex());
      QString cacheFilePath = cacheDirPath + "/" + fileHash + ".png";
      QPixmap thumb;

      // 检查本地缓存文件是否存在
      QFileInfo cacheFileInfo(cacheFilePath);

      if (cacheFileInfo.exists()) {
        // 从缓存文件加载缩略图
        thumb.load(cacheFilePath);

        // 添加到内存缓存
        if (!thumb.isNull()) {
          thumbnailMutex.lock();
          int sizeInBytes = thumb.width() * thumb.height() * thumb.depth() / 8;
          thumbnailCache.insert(path, new QPixmap(thumb), sizeInBytes);
          thumbnailMutex.unlock();

          batchResults.append(qMakePair(path, thumb));

          // 批量处理
          if (batchResults.size() >= batchSize) {
            processBatchThumbnails(batchResults);
            batchResults.clear();
          }
          continue;
        }
      }

      // 检查内存缓存
      if (thumbnailCache.contains(path)) {
        batchResults.append(qMakePair(path, *thumbnailCache[path]));

        // 批量处理
        if (batchResults.size() >= batchSize) {
          processBatchThumbnails(batchResults);
          batchResults.clear();
        }
        continue;
      }

      // 生成缩略图
      thumb = generateThumbnail(path);
      if (!thumb.isNull()) {
        // 保存到本地缓存
        thumb.save(cacheFilePath, "PNG");

        // 添加到内存缓存
        thumbnailMutex.lock();
        int sizeInBytes = thumb.width() * thumb.height() * thumb.depth() / 8;
        thumbnailCache.insert(path, new QPixmap(thumb), sizeInBytes);
        thumbnailMutex.unlock();

        batchResults.append(qMakePair(path, thumb));

        // 批量处理
        if (batchResults.size() >= batchSize) {
          processBatchThumbnails(batchResults);
          batchResults.clear();
        }
      }

      // 每处理 15 个休息一下，避免 UI 卡顿
      if (i % 15 == 0) {
        QThread::msleep(15);
      }
    }

    // 处理剩余的缩略图
    if (!batchResults.isEmpty()) {
      processBatchThumbnails(batchResults);
    }
  });

  thumbnailWatcher->setFuture(future);
}

void ImageViewer::onThumbnailLoaded(const QString &path,
                                    const QPixmap &pixmap) {
  for (int i = 0; i < thumbnailList->count(); ++i) {
    QListWidgetItem *item = thumbnailList->item(i);
    if (item->data(Qt::UserRole).toString() == path) {
      item->setIcon(QIcon(pixmap));
      // 强制更新布局以确保网格对齐
      thumbnailList->update();
      // 使用公共方法触发布局更新
      thumbnailList->setGridSize(thumbnailList->gridSize());
      break;
    }
  }
}

void ImageViewer::onThumbnailClicked(QListWidgetItem *item) {
  QString newPath = item->data(Qt::UserRole).toString();
  if (newPath == currentPath)
    return;

  // 找到新图片索引
  for (int i = 0; i < imageFiles.size(); ++i) {
    if (imageFiles[i].absoluteFilePath() == newPath) {
      currentImageIndex = i;
      break;
    }
  }

  currentPath = newPath;
  setWindowTitle(QFileInfo(newPath).fileName());

  // 检查是否是 GIF 文件
  bool newIsGif = newPath.toLower().endsWith(".gif");

  // 检查是否是 WebP 文件
  bool newIsWebp = newPath.toLower().endsWith(".webp");

  // 如果从 GIF 切换到普通图片或反之，需要重置相关状态
  if (isGif != newIsGif) {
    // 停止当前 GIF 动画（如果是）
    if (isGif && gifMovie) {
      gifMovie->stop();
      delete gifMovie;
      gifMovie = nullptr;
    }

    isGif = newIsGif;

    if (isGif) {
      // 初始化 GIF 动画
      setupGifAnimation();
    } else if (newIsWebp) {
      // 加载 WebP 图片
      isWebp = true;
      loadWebpImage();
    } else {
      // 加载普通图片
      originalPixmap = QPixmap(currentPath);
      rotationAngle = 0;
      rotatedPixmap = originalPixmap;
    }
  } else if (isGif) {
    // 切换到新的 GIF
    setupGifAnimation();
  } else if (isWebp != newIsWebp) {
    // 停止当前 WebP 动画（如果是）
    if (isWebp && webpAnimationTimer) {
      webpAnimationTimer->stop();
      delete webpAnimationTimer;
      webpAnimationTimer = nullptr;
    }

    isWebp = newIsWebp;

    if (isWebp) {
      // 加载 WebP 图片
      loadWebpImage();
    } else {
      // 加载普通图片
      originalPixmap = QPixmap(currentPath);
      rotationAngle = 0;
      rotatedPixmap = originalPixmap;
    }
  } else if (isWebp) {
    // 切换到新的 WebP
    loadWebpImage();
  } else {
    // 切换到新的普通图片
    originalPixmap = QPixmap(currentPath);
    rotationAngle = 0;
    rotatedPixmap = originalPixmap;
  }

  offset = QPointF(0, 0);
  updateImageDisplay();
  hideThumbnailMenu();
}

void ImageViewer::showThumbnailMenu() {
  if (imageFiles.size() <= 1)
    return; // 只有一张图时不显示

  int menuHeight = 120;
  thumbnailScrollArea->setGeometry(0, height() - menuHeight, width(),
                                   menuHeight);
  thumbnailScrollArea->setVisible(true);
  thumbnailsVisible = true;

  // 确保缩略图列表布局已更新
  thumbnailList->updateGeometry();
  thumbnailList->update();
  // 强制重新计算布局以确保网格对齐
  thumbnailList->setGridSize(
      thumbnailList->gridSize()); // 使用公共方法触发布局更新

  // 滚动到当前选中项
  if (thumbnailList->currentItem()) {
    // 使用 QTimer 确保在布局更新后滚动
    QTimer::singleShot(50, this, [this]() { // 增加延时确保布局完全更新
      if (thumbnailList->currentItem()) {
        thumbnailList->scrollToItem(thumbnailList->currentItem(),
                                    QAbstractItemView::PositionAtCenter);
      }
    });
  }
}

void ImageViewer::hideThumbnailMenu() {
  thumbnailScrollArea->setVisible(false);
  thumbnailsVisible = false;
}

bool ImageViewer::eventFilter(QObject *obj, QEvent *event) {
  if (obj == thumbnailList) {
    // 处理触摸事件
    if (event->type() == QEvent::TouchBegin) {
      showZoomButtons(); // 显示操作按钮
      return true;
    }
  }
  return QWidget::eventFilter(obj, event);
}

void ImageViewer::switchToImage(int index) {
  if (index >= 0 && index < imageFiles.size()) {
    currentImageIndex = index;
    currentPath = imageFiles[index].absoluteFilePath();
    setWindowTitle(QFileInfo(currentPath).fileName());

    // 检查是否是 GIF 文件
    bool newIsGif = currentPath.toLower().endsWith(".gif");

    // 检查是否是 WebP 文件
    bool newIsWebp = currentPath.toLower().endsWith(".webp");

    // 如果从 GIF 切换到普通图片或反之，需要重置相关状态
    if (isGif != newIsGif) {
      // 停止当前 GIF 动画（如果是）
      if (isGif && gifMovie) {
        gifMovie->stop();
        delete gifMovie;
        gifMovie = nullptr;
      }

      isGif = newIsGif;

      if (isGif) {
        // 初始化 GIF 动画
        setupGifAnimation();
      } else if (newIsWebp) {
        // 加载 WebP 图片
        isWebp = true;
        loadWebpImage();
      } else {
        // 加载普通图片
        originalPixmap = QPixmap(currentPath);
        rotationAngle = 0;
        rotatedPixmap = originalPixmap;
      }
    } else if (isGif) {
      // 切换到新的 GIF
      setupGifAnimation();
    } else if (isWebp != newIsWebp) {
      // 停止当前 WebP 动画（如果是）
      if (isWebp && webpAnimationTimer) {
        webpAnimationTimer->stop();
        delete webpAnimationTimer;
        webpAnimationTimer = nullptr;
      }

      isWebp = newIsWebp;

      if (isWebp) {
        // 加载 WebP 图片
        loadWebpImage();
      } else {
        // 加载普通图片
        originalPixmap = QPixmap(currentPath);
        rotationAngle = 0;
        rotatedPixmap = originalPixmap;
      }
    } else if (isWebp) {
      // 切换到新的 WebP
      loadWebpImage();
    } else {
      // 切换到新的普通图片
      originalPixmap = QPixmap(currentPath);
      rotationAngle = 0;
      rotatedPixmap = originalPixmap;
    }

    offset = QPointF(0, 0);
    updateImageDisplay();
  }
}

// 处理批量缩略图更新
void ImageViewer::processBatchThumbnails(
    const QList<QPair<QString, QPixmap>> &batch) {
  QMetaObject::invokeMethod(
      this,
      [this, batch]() {
        for (const auto &pair : batch) {
          onThumbnailLoaded(pair.first, pair.second);
        }
      },
      Qt::QueuedConnection);
}

// 设置 GIF 动画
void ImageViewer::setupGifAnimation() {
  // 停止并删除旧的 GIF 动画（如果有）
  if (gifMovie) {
    gifMovie->stop();
    delete gifMovie;
    gifMovie = nullptr;
  }

  // 创建新的 GIF 动画
  gifMovie = new QMovie(currentPath);
  if (!gifMovie->isValid()) {
    delete gifMovie;
    gifMovie = nullptr;
    return;
  }

  // 连接帧更新信号
  connect(gifMovie, &QMovie::frameChanged, this, &ImageViewer::updateGifFrame);

  // 获取第一帧
  currentGifFrame = gifMovie->currentPixmap();

  // 开始播放 GIF
  gifMovie->start();
}

// 更新 GIF 帧
void ImageViewer::updateGifFrame() {
  if (gifMovie && gifMovie->isValid()) {
    currentGifFrame = gifMovie->currentPixmap();
    update(); // 触发重绘
  }
}

// 加载 WebP 图片
void ImageViewer::loadWebpImage() {
  // 先停止并删除旧的动画计时器（如果有）
  if (webpAnimationTimer) {
    webpAnimationTimer->stop();
    delete webpAnimationTimer;
    webpAnimationTimer = nullptr;
  }

  // 重置动画标志
  isAnimatedWebp = false;
  currentWebpFrame = 0;
  currentLoopCount = 0;

  QFile file(currentPath);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "无法打开 WebP 文件:" << currentPath;
    return;
  }

  QByteArray data = file.readAll();
  file.close();

  if (data.isEmpty()) {
    qWarning() << "WebP 文件为空:" << currentPath;
    return;
  }

  // 检查是否是动画 WebP
  WebPData webp_data;
  webp_data.bytes = reinterpret_cast<const uint8_t *>(data.constData());
  webp_data.size = data.size();

  WebPDemuxer *demux = WebPDemux(&webp_data);
  if (demux) {
    // 动画 WebP 处理
    setupWebpAnimation(demux, data);
    WebPDemuxDelete(demux);
  } else {
    // 静态 WebP 处理
    int width, height;
    uint8_t *decoded_data =
        WebPDecodeRGBA(reinterpret_cast<const uint8_t *>(data.constData()),
                       data.size(), &width, &height);

    if (decoded_data) {
      // 创建 QImage 从解码后的数据
      QImage image(decoded_data, width, height, QImage::Format_RGBA8888);
      // 创建 QPixmap 并复制数据，因为 decoded_data 会在后面释放
      originalPixmap = QPixmap::fromImage(image.copy());
      rotatedPixmap = originalPixmap;
      // 释放解码后的数据
      WebPFree(decoded_data);
    } else {
      qWarning() << "WebP 解码失败:" << currentPath;
    }
  }
}

// 设置 WebP 动画
void ImageViewer::setupWebpAnimation(void *demuxPtr, const QByteArray &data) {
  if (!demuxPtr) {
    qWarning() << "WebP demuxer 为空";
    return;
  }

  WebPDemuxer *demux = static_cast<WebPDemuxer *>(demuxPtr);
  // 获取动画信息
  uint32_t flags = WebPDemuxGetI(demux, WEBP_FF_FORMAT_FLAGS);
  isAnimatedWebp = (flags & ANIMATION_FLAG) != 0;

  if (!isAnimatedWebp) {
    // 不是动画 WebP，作为静态图片处理
    int width, height;
    uint8_t *decoded_data =
        WebPDecodeRGBA(reinterpret_cast<const uint8_t *>(data.constData()),
                       data.size(), &width, &height);

    if (decoded_data) {
      QImage image(decoded_data, width, height, QImage::Format_RGBA8888);
      originalPixmap = QPixmap::fromImage(image.copy());
      rotatedPixmap = originalPixmap;
      WebPFree(decoded_data);
    } else {
      qWarning() << "WebP 解码失败:" << currentPath;
    }
    return;
  }

  // 获取动画参数
  webpFrameCount = WebPDemuxGetI(demux, WEBP_FF_FRAME_COUNT);
  webpLoopCount = WebPDemuxGetI(demux, WEBP_FF_LOOP_COUNT);
  webpBackgroundColor = WebPDemuxGetI(demux, WEBP_FF_BACKGROUND_COLOR);
  webpCanvasWidth = WebPDemuxGetI(demux, WEBP_FF_CANVAS_WIDTH);
  webpCanvasHeight = WebPDemuxGetI(demux, WEBP_FF_CANVAS_HEIGHT);

  // 存储原始数据
  webpData = data;

  // 初始化动画参数
  currentWebpFrame = 0;
  currentLoopCount = 0; // 初始化循环计数

  // 停止并删除旧的计时器（如果有）
  if (webpAnimationTimer) {
    webpAnimationTimer->stop();
    delete webpAnimationTimer;
    webpAnimationTimer = nullptr;
  }

  // 创建新的计时器
  webpAnimationTimer = new QTimer(this);
  connect(webpAnimationTimer, &QTimer::timeout, this,
          &ImageViewer::updateWebpFrame);

  // 加载第一帧
  loadWebpFrame(0);

  // 开始动画
  if (webpFrameCount > 1) {
    webpAnimationTimer->start();
  }
}

// 加载 WebP 帧
void ImageViewer::loadWebpFrame(int frameIndex) {
  if (webpData.isEmpty())
    return;

  WebPData webp_data;
  webp_data.bytes = reinterpret_cast<const uint8_t *>(webpData.constData());
  webp_data.size = webpData.size();

  WebPDemuxer *demux = WebPDemux(&webp_data);
  if (!demux) {
    qWarning() << "无法创建 WebP demuxer";
    return;
  }

  // 获取指定帧
  WebPIterator iter;
  if (!WebPDemuxGetFrame(demux, frameIndex + 1, &iter)) {
    qWarning() << "无法获取 WebP 帧:" << frameIndex;
    WebPDemuxReleaseIterator(&iter);
    WebPDemuxDelete(demux);
    return;
  }

  // 解码帧
  int width, height;
  uint8_t *decoded_data =
      WebPDecodeRGBA(iter.fragment.bytes, iter.fragment.size, &width, &height);

  if (decoded_data) {
    QImage image(decoded_data, width, height, QImage::Format_RGBA8888);
    currentWebpFramePixmap = QPixmap::fromImage(image.copy());
    WebPFree(decoded_data);

    // 设置下一帧的延迟时间
    if (webpAnimationTimer) {
      webpAnimationTimer->setInterval(iter.duration > 0 ? iter.duration : 100);
    }
  } else {
    qWarning() << "无法解码 WebP 帧:" << frameIndex;
  }

  WebPDemuxReleaseIterator(&iter);
  WebPDemuxDelete(demux);
}

// 更新 WebP 帧
void ImageViewer::updateWebpFrame() {
  if (!isAnimatedWebp || webpFrameCount <= 0 || !webpAnimationTimer)
    return;

  // 更新到下一帧
  currentWebpFrame = (currentWebpFrame + 1) % webpFrameCount;

  // 如果循环计数为 0（无限循环）或还有循环次数
  if (webpLoopCount == 0 || currentLoopCount < webpLoopCount) {
    if (currentWebpFrame == 0) {
      currentLoopCount++;
    }
    loadWebpFrame(currentWebpFrame);
    update(); // 触发重绘
  } else {
    // 动画结束
    webpAnimationTimer->stop();
  }
}