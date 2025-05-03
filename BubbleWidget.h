#ifndef BUBBLEWIDGET_H
#define BUBBLEWIDGET_H

#include <QWidget>
#include <QPoint>
#include "FileManagerWindow.h"

class BubbleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BubbleWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QPoint m_dragPosition;
    QPoint m_pressPos;       // 新增：记录按下时的位置
    bool m_isPressed = false;
    bool m_isHovered = false;
    QPixmap m_icon;

    FileManagerWindow *MyWindow = nullptr; // 缓存窗口指针
};

#endif // BUBBLEWIDGET_H
