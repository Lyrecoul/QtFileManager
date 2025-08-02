#include "ToggleSwitch.h"
#include <QEasingCurve>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>

ToggleSwitch::ToggleSwitch(QWidget *parent)
    : QWidget(parent), m_checked(true), m_offset(0) {
  setCursor(Qt::PointingHandCursor);
  // offset 会在 resizeEvent 里自动设置
}

QSize ToggleSwitch::sizeHint() const {
  return QSize(50, 30); // 可根据实际调整
}

QSize ToggleSwitch::minimumSizeHint() const { return QSize(40, 24); }

void ToggleSwitch::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QColor bgColor = m_checked ? QColor("#4cd964") : QColor("#aaaaaa");
  p.setBrush(bgColor);
  p.setPen(Qt::NoPen);
  QRectF rect = this->rect();
  p.drawRoundedRect(rect, height() / 2.0, height() / 2.0);

  p.setBrush(Qt::white);
  qreal radius = height() - 4;
  QRectF circle(m_offset, 2, radius, radius);
  p.drawEllipse(circle);
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    setChecked(!m_checked);
    emit toggled(m_checked);
  }
}

void ToggleSwitch::setChecked(bool checked) {
  if (m_checked == checked)
    return;

  m_checked = checked;

  QPropertyAnimation *animation = new QPropertyAnimation(this, "offset");
  animation->setDuration(150);
  animation->setEasingCurve(QEasingCurve::InOutQuad);
  qreal end = m_checked ? width() - height() + 2 : 2;
  animation->setStartValue(m_offset);
  animation->setEndValue(end);
  animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToggleSwitch::setOffset(qreal value) {
  m_offset = value;
  update();
}

qreal ToggleSwitch::offset() const { return m_offset; }

void ToggleSwitch::resizeEvent(QResizeEvent *) {
  // 确保初始化时滑块在正确位置
  m_offset = m_checked ? width() - height() + 2 : 2;
  update();
}