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
#include <QGraphicsOpacityEffect>

ImageViewer::ImageViewer(const QString &path, QWidget *parent)
    : QWidget(parent), currentPath(path)
{
    // 窗口设置
    setWindowTitle(QFileInfo(path).fileName());
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 图片标签
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel->setScaledContents(false); // 重要：禁用自动缩放

    // 关闭按钮
    closeButton = new QPushButton(this);
    closeButton->setFixedSize(30, 30);
    closeButton->setIcon(QIcon(":/icons/close.png"));
    closeButton->setIconSize(QSize(20, 20));
    closeButton->setStyleSheet(
        "QPushButton {"
        "   background: rgba(0, 0, 0, 50);"
        "   border: 1px solid rgba(255, 255, 255, 100);"
        "   border-radius: 15px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(0, 0, 0, 70);"
        "}");

    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(this);
    opacityEffect->setOpacity(0.5);
    closeButton->setGraphicsEffect(opacityEffect);

    // 布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->setContentsMargins(0, 10, 10, 0);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(imageLabel);
    mainLayout->addLayout(buttonLayout);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);

    // 初始加载图片但不立即调整大小
    updateImageDisplay();
}

void ImageViewer::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (firstShow) {
        adjustWindowSize();
        firstShow = false;
    }
}

void ImageViewer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateImageDisplay();
}

void ImageViewer::adjustWindowSize()
{
    QPixmap pixmap(currentPath);
    if (!pixmap.isNull()) {
        QSize screenSize = qApp->primaryScreen()->availableSize();
        QSize imageSize = pixmap.size();

        // 计算适合屏幕的窗口大小
        double scale = qMin(
            (double)screenSize.width() * 0.9 / imageSize.width(),
            (double)screenSize.height() * 0.9 / imageSize.height()
            );

        QSize windowSize = imageSize * scale;
        resize(windowSize);

        // 居中窗口
        move(
            (screenSize.width() - windowSize.width()) / 2,
            (screenSize.height() - windowSize.height()) / 2
            );
    }
}

void ImageViewer::updateImageDisplay()
{
    QPixmap pixmap(currentPath);
    if (pixmap.isNull()) {
        imageLabel->setText("无法加载图片");
    } else {
        // 保持宽高比缩放图片以适应标签
        QPixmap scaled = pixmap.scaled(
            imageLabel->size(),
            Qt::KeepAspectRatioByExpanding,
            Qt::SmoothTransformation
            );
        imageLabel->setPixmap(scaled);
    }
}
