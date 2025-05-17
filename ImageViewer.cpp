#include "ImageViewer.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QResizeEvent>
#include <QFileInfo>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <QStyleOption>
#include <QGraphicsOpacityEffect>

ImageViewer::ImageViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path)
{
    setWindowTitle(QFileInfo(path).fileName());
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // 强制设置为全屏大小
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenRect = screen->geometry();
        setGeometry(screenRect);
        resize(screenRect.size());
    }

    // 加载图片
    originalPixmap = QPixmap(currentPath);
    rotatedPixmap = originalPixmap;

    // 缩放按钮
    zoomInButton = new QPushButton("＋", this);
    zoomOutButton = new QPushButton("－", this);
    zoomInButton->setVisible(false);
    zoomOutButton->setVisible(false);

    QString btnStyle =
        "QPushButton {"
        "   color: white;"
        "   font-size: 22px;"
        "   background: rgba(0,0,0,120);"
        "   border-radius: 16px;"
        "   padding: 4px 12px;"
        "   opacity: 0.7;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(0,0,0,180);"
        "   opacity: 1.0;"
        "}";
    zoomInButton->setStyleSheet(btnStyle);
    zoomOutButton->setStyleSheet(btnStyle);

    zoomInButton->setFixedSize(40, 32);
    zoomOutButton->setFixedSize(40, 32);

    connect(zoomInButton, &QPushButton::clicked, this, &ImageViewer::onZoomIn);
    connect(zoomOutButton, &QPushButton::clicked, this, &ImageViewer::onZoomOut);

    connect(&zoomButtonHideTimer, &QTimer::timeout, this, &ImageViewer::hideZoomButtons);

    // 长按检测
    longPressTimer.setSingleShot(true);
    connect(&longPressTimer, &QTimer::timeout, [this]() {
        longPressDetected = true;
        close();
    });

    // 初始显示
    updateImageDisplay();
}

void ImageViewer::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 强制全屏并铺满
    if (!isFullScreen())
        showFullScreen();
    // 再次确保窗口大小与屏幕一致
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenRect = screen->geometry();
        setGeometry(screenRect);
        resize(screenRect.size());
    }
    updateImageDisplay();
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateImageDisplay();
}

void ImageViewer::paintEvent(QPaintEvent *event)
{
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
    QPointF topLeft = QPointF(width() / 2.0, height() / 2.0) - QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) + offset;

    // 限制偏移不超出图片边界
    QSizeF viewSize(width(), height());
    QSizeF imgSize = scaledSize;
    QPointF minOffset(
        qMin(0.0, (viewSize.width() - imgSize.width()) / 2.0),
        qMin(0.0, (viewSize.height() - imgSize.height()) / 2.0)
    );
    QPointF maxOffset(
        qMax(0.0, (imgSize.width() - viewSize.width()) / 2.0),
        qMax(0.0, (imgSize.height() - viewSize.height()) / 2.0)
    );
    offset.setX(qBound(minOffset.x(), offset.x(), maxOffset.x()));
    offset.setY(qBound(minOffset.y(), offset.y(), maxOffset.y()));

    topLeft = QPointF(width() / 2.0, height() / 2.0) - QPointF(scaledSize.width() / 2.0, scaledSize.height() / 2.0) + offset;

    painter.fillRect(rect(), Qt::black);
    painter.drawPixmap(QRectF(topLeft, scaledSize), rotatedPixmap, QRectF(0, 0, rotatedPixmap.width(), rotatedPixmap.height()));

    // 缩放按钮位置
    int margin = 12;
    zoomInButton->move(width() / 2 - zoomInButton->width() - margin / 2, margin);
    zoomOutButton->move(width() / 2 + margin / 2, margin);
}

void ImageViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        lastMousePos = event->pos();
        dragging = true;
        longPressDetected = false;
        longPressTimer.start(600); // 600ms长按关闭
    }
}

void ImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging) {
        QPointF delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
        if (longPressTimer.isActive())
            longPressTimer.stop(); // 移动则不判定为长按
    }
}

void ImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (dragging) {
        dragging = false;
        if (longPressTimer.isActive())
            longPressTimer.stop();
        if (!longPressDetected) {
            // 普通点击，显示缩放按钮
            showZoomButtons();
        }
    }
}

void ImageViewer::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        rotateImage();
        updateImageDisplay();
    }
}

void ImageViewer::onZoomIn()
{
    scaleFactor *= 1.25;
    if (scaleFactor > 5.0) scaleFactor = 5.0;
    update();
    showZoomButtons();
}

void ImageViewer::onZoomOut()
{
    // 计算最小缩放比例，保证图片完整显示
    double minScale = 0.2;
    if (!rotatedPixmap.isNull()) {
        double scaleW = double(width()) / rotatedPixmap.width();
        double scaleH = double(height()) / rotatedPixmap.height();
        minScale = qMin(scaleW, scaleH);
        minScale = qMin(minScale, 1.0); // 不允许初始就放大
        minScale = qMax(minScale, 0.05); // 防止极小
    }
    scaleFactor /= 1.25;
    if (scaleFactor < minScale) scaleFactor = minScale;
    update();
    showZoomButtons();
}

void ImageViewer::showZoomButtons()
{
    zoomInButton->setVisible(true);
    zoomOutButton->setVisible(true);
    zoomButtonHideTimer.start(1800); // 1.8秒后自动隐藏
}

void ImageViewer::hideZoomButtons()
{
    zoomInButton->setVisible(false);
    zoomOutButton->setVisible(false);
}

void ImageViewer::updateImageDisplay()
{
    // 缩放和旋转已在paintEvent处理
    update();
}

void ImageViewer::rotateImage()
{
    rotationAngle = (rotationAngle + 90) % 360;
    QTransform trans;
    trans.rotate(rotationAngle);
    rotatedPixmap = originalPixmap.transformed(trans, Qt::SmoothTransformation);
    // 旋转后重置偏移
    offset = QPointF(0, 0);

    // 旋转后自动调整缩放比例，保证图片完整显示
    if (!rotatedPixmap.isNull()) {
        double scaleW = double(width()) / rotatedPixmap.width();
        double scaleH = double(height()) / rotatedPixmap.height();
        double minScale = qMin(scaleW, scaleH);
        minScale = qMin(minScale, 1.0);
        minScale = qMax(minScale, 0.05);
        if (scaleFactor < minScale) scaleFactor = minScale;
    }
}
