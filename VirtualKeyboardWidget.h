#ifndef VIRTUALKEYBOARDWIDGET_H
#define VIRTUALKEYBOARDWIDGET_H

#include "qevent.h"
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWidget>

class VirtualKeyboardWidget : public QWidget {
  Q_OBJECT
public:
  explicit VirtualKeyboardWidget(QWidget *parent = nullptr);
  explicit VirtualKeyboardWidget(const QString &defaultText, const QString &placeholderText = "", QWidget *parent = nullptr);
  QString text() const;
  void setText(const QString &text);
  void setPlaceholderText(const QString &text);

signals:
  void textEntered(const QString &text);
  void cancelled();

private:
  enum KeyboardPage { Letters, Numbers, Symbols };
  KeyboardPage currentPage;

  QLineEdit *inputLine;
  QVBoxLayout *keyLayout;
  QWidget *keyboardWidget;

  QScrollArea *scrollArea;

  QVector<QStringList> letterKeys;
  QVector<QStringList> numberKeys;
  QVector<QStringList> symbolKeys;

  void setupUI();
  void initializeKeyboard(const QString &defaultText, const QString &placeholderText);
  void buildKeyboard();
  QPushButton *createButton(const QString &text);

  QVector<QVector<QPushButton *>> keyButtons; // 用于复用按钮
  const int maxRows = 6;
  const int maxCols = 5;
};

class MyButton : public QPushButton {
    QPoint pressPos;
    bool moved = false;
    bool pressedInside = false;

public:
    explicit MyButton(const QString &text, QWidget *parent = nullptr)
        : QPushButton(text, parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override {
        pressPos = event->pos();
        moved = false;
        pressedInside = rect().contains(pressPos);
        // 不立即传给父类，先判断是否滑动
        // 不调用 QPushButton::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if ((event->pos() - pressPos).manhattanLength() > 10) {
            moved = true;
        }
        // 不传给父类，避免滑动中触发点击动画
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (!moved && pressedInside && rect().contains(event->pos())) {
            emit clicked();  // 只在无滑动且点击范围内才触发
        }
        // 不传给 QPushButton::mouseReleaseEvent(event);
    }
};

#endif // VIRTUALKEYBOARDWIDGET_H
