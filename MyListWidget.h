// MyListWidget.h
#pragma once

#include <QListWidget>
#include <QMouseEvent>
#include <QApplication>

class MyListWidget : public QListWidget {
  Q_OBJECT
public:
  explicit MyListWidget(QWidget *parent = nullptr)
      : QListWidget(parent), m_isDragging(false) {}

protected:
  void mousePressEvent(QMouseEvent *event) override {
    m_pressPos = event->pos();
    m_isDragging = false;
    QListWidget::mousePressEvent(event);
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    if ((event->pos() - m_pressPos).manhattanLength() > QApplication::startDragDistance()) {
      m_isDragging = true;
    }
    QListWidget::mouseMoveEvent(event);
  }

  void mouseReleaseEvent(QMouseEvent *event) override {
    if (!m_isDragging) {
      // 只有非滑动时才执行释放事件，触发点击
      QListWidget::mouseReleaseEvent(event);
    } else {
      // 滑动时不触发点击事件，忽略
      event->ignore();
    }
  }

private:
  QPoint m_pressPos;
  bool m_isDragging;
};
