#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QWidget>

class ToggleSwitch : public QWidget {
  Q_OBJECT
  Q_PROPERTY(qreal offset READ offset WRITE setOffset)

public:
  explicit ToggleSwitch(QWidget *parent = nullptr);

  void setChecked(bool checked);
  bool isChecked() const { return m_checked; }

  void setOffset(qreal value);
  qreal offset() const;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

signals:
  void toggled(bool checked);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  bool m_checked;
  qreal m_offset;
};

#endif // TOGGLESWITCH_H