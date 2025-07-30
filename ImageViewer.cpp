#include "ImageViewer.h"

#include <QApplication>
#include <QFileInfo>
#include <QGestureEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPinchGesture>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QTransform>

ImageViewer::ImageViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path) {
  setWindowTitle(QFileInfo(path).fileName());
  setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_DeleteOnClose);

  // 加载图片
  originalPixmap = QPixmap(currentPath);
  rotatedPixmap = originalPixmap;

  if (originalPixmap.isNull()) {
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

  closeButton = new QPushButton("✕", this);
  closeButton->setVisible(false);
  closeButton->setStyleSheet(buttonStyle());
  closeButton->setFixedSize(40, 32);
  connect(closeButton, &QPushButton::clicked, this, &ImageViewer::close);

  grabGesture(Qt::PinchGesture);
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
}

void ImageViewer::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

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

  int margin = 12;
  int totalWidth = zoomInButton->width() + zoomOutButton->width() +
                   rotateButton->width() + closeButton->width() + 3 * margin;

  int baseX = (width() - totalWidth) / 2;

  zoomInButton->move(baseX, margin);
  zoomOutButton->move(baseX + zoomInButton->width() + margin, margin);
  rotateButton->move(baseX + zoomInButton->width() + zoomOutButton->width() +
                         2 * margin,
                     margin);
  closeButton->move(baseX + zoomInButton->width() + zoomOutButton->width() +
                        rotateButton->width() + 3 * margin,
                    margin);
}

void ImageViewer::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    lastMousePos = event->pos();
    dragging = true;
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
  if (!rotatedPixmap.isNull()) {
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
  closeButton->setVisible(true);
  zoomButtonHideTimer.start(1800);
}

void ImageViewer::hideZoomButtons() {
  zoomInButton->setVisible(false);
  zoomOutButton->setVisible(false);
  rotateButton->setVisible(false);
  closeButton->setVisible(false);
}

void ImageViewer::updateImageDisplay() {
  if (!rotatedPixmap.isNull()) {
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
  rotatedPixmap = originalPixmap.transformed(trans, Qt::SmoothTransformation);
  offset = QPointF(0, 0);
}
