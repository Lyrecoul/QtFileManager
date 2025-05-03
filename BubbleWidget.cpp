#include "BubbleWidget.h"
#include "FileManagerWindow.h"
#include <QPainter>
#include <QMouseEvent>

BubbleWidget::BubbleWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_icon.load(":/icons/icon.png");
    resize(45, 45);
}

void BubbleWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor bgColor;
    if (m_isPressed) {
        bgColor = QColor(0, 120, 215, 200);
    } else if (m_isHovered) {
        bgColor = QColor(0, 150, 255, 180);
    } else {
        bgColor = QColor(0, 150, 255, 160);
    }

    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(rect());

    if (!m_icon.isNull()) {
        int iconSize = qMin(width(), height()) * 0.6;
        QRect iconRect((width() - iconSize) / 2,
                      (height() - iconSize) / 2,
                      iconSize, iconSize);
        painter.drawPixmap(iconRect, m_icon);
    }
}

void BubbleWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
        m_isPressed = true;
        m_pressPos = event->globalPos(); // 记录按下时的位置
        update();
        event->accept();
    }
}

void BubbleWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPos() - m_dragPosition);
        event->accept();
    }
}

void BubbleWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isPressed = false;
        update();

        // 计算移动距离
        QPoint releasePos = event->globalPos();
        QPoint diff = releasePos - m_pressPos;
        int distance = diff.manhattanLength(); // 曼哈顿距离

        // 如果移动距离小于5像素，认为是点击
        if (distance < 5) {
            if (!MyWindow) {
                MyWindow = new FileManagerWindow(this); // 首次创建
                MyWindow->setAttribute(Qt::WA_DeleteOnClose, false); // 防止关闭时被销毁
            }
            MyWindow->showFullScreen();  // 显示窗口
            MyWindow->activateWindow(); // 激活到前台
        }
        event->accept();
    }
}

void BubbleWidget::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = true;
    update();
}

void BubbleWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = false;
    update();
}
