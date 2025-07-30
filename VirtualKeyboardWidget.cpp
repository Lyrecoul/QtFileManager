#include "VirtualKeyboardWidget.h"
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QScroller>

VirtualKeyboardWidget::VirtualKeyboardWidget(QWidget *parent)
    : QWidget(parent), currentPage(Letters) {
  setFixedSize(320, 170);
  setStyleSheet(R"(QWidget { background-color: #111; color: white; })");

  // 确保主窗口背景不透明
  this->setAutoFillBackground(true);
  QPalette pal = palette();
  pal.setColor(QPalette::Window, QColor("#111")); // 深色不透明背景
  this->setPalette(pal);

  // 顶部输入行
  inputLine = new QLineEdit(this);
  inputLine->setStyleSheet(
      "font-size: 16px; background-color: #222; color: white; padding: 4px;");
  inputLine->setReadOnly(false); // 让游标显示
  inputLine->setFocus();

  QPushButton *btnOk = new QPushButton("✓", this);
  QPushButton *btnClose = new QPushButton("✕", this);
  btnOk->setFixedSize(32, 32);
  btnClose->setFixedSize(32, 32);
  btnOk->setStyleSheet(
      "background-color: #444; border: none; border-radius: 6px;");
  btnClose->setStyleSheet(
      "background-color: #444; border: none; border-radius: 6px;");

  connect(btnOk, &QPushButton::clicked, this, [=]() {
    emit textEntered(inputLine->text());
    this->hide();
  });
  connect(btnClose, &QPushButton::clicked, this, [=]() {
    emit cancelled();
    this->hide();
  });

  QHBoxLayout *topLayout = new QHBoxLayout;
  topLayout->addWidget(inputLine);
  topLayout->addWidget(btnOk);
  topLayout->addWidget(btnClose);
  topLayout->setContentsMargins(4, 4, 4, 0);

  // Tab 分组按钮
  QWidget *tabWidget = new QWidget(this);
  tabWidget->setFixedHeight(40); // 强制高度，防止下面出现空白
  tabWidget->setStyleSheet("background-color: #111;");

  QHBoxLayout *tabLayout = new QHBoxLayout(tabWidget);
  tabLayout->setContentsMargins(4, 0, 4, 0);
  tabLayout->setSpacing(4);

  QStringList tabs = {"字母", "数字", "符号"};
  for (int i = 0; i < tabs.size(); ++i) {
    QPushButton *tabBtn = new QPushButton(tabs[i]);
    tabBtn->setCheckable(true);
    tabBtn->setAutoExclusive(true);
    tabBtn->setStyleSheet(R"(
    QPushButton {
      background-color: #222; color: white;
      padding: 6px; border-radius: 6px;
    }
    QPushButton:checked {
      background-color: #555;
    })");

    if (i == 0)
      tabBtn->setChecked(true);

    connect(tabBtn, &QPushButton::clicked, this, [=]() {
      currentPage = static_cast<KeyboardPage>(i);
      buildKeyboard();
    });

    tabLayout->addWidget(tabBtn);
  }

  // 键盘区域
  scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);
  scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏滚动条
  scrollArea->setStyleSheet(
      "QScrollArea { background-color: #111; border: none; }");

  keyboardWidget = new QWidget(this);
  keyLayout = new QVBoxLayout(keyboardWidget);
  keyLayout->setContentsMargins(4, 4, 4, 4);
  keyLayout->setSpacing(4);

  scrollArea->setWidget(keyboardWidget);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(scrollArea);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(mainLayout);

  // 初始化按键内容
  letterKeys = {{"A", "B", "C", "D", "E"}, {"F", "G", "H", "I", "J"},
                {"K", "L", "M", "N", "O"}, {"P", "Q", "R", "S", "T"},
                {"U", "V", "W", "X", "Y"}, {"Z", "←", "空格", "<", ">"}};

  numberKeys = {{"1", "2", "3", "4", "5"},
                {"6", "7", "8", "9", "0"},
                {"+", "-", "*", "/", "="},
                {"←", "空格", "<", ">", "OK"}};

  symbolKeys = {{"!", "@", "#", "$", "%"},
                {"^", "&", "*", "(", ")"},
                {"[", "]", "{", "}", "\\"},
                {"←", "空格", "<", ">", "OK"}};

  QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);
  scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
  scrollArea->viewport()->setAttribute(Qt::WA_TransparentForMouseEvents, false);

  for (int row = 0; row < maxRows; ++row) {
    QHBoxLayout *rowLayout = new QHBoxLayout;
    QVector<QPushButton *> rowButtons;
    for (int col = 0; col < maxCols; ++col) {
      QPushButton *btn = createButton(""); // 空按钮
      btn->setVisible(false);              // 初始不可见
      rowLayout->addWidget(btn);
      rowButtons.append(btn);
    }
    keyLayout->addLayout(rowLayout);
    keyButtons.append(rowButtons);
  }

  buildKeyboard();
}

void VirtualKeyboardWidget::buildKeyboard() {
  const QVector<QStringList> *keys = nullptr;
  switch (currentPage) {
  case Letters:
    keys = &letterKeys;
    break;
  case Numbers:
    keys = &numberKeys;
    break;
  case Symbols:
    keys = &symbolKeys;
    break;
  }

  int row = 0;
  for (; row < keys->size(); ++row) {
    const QStringList &keyRow = (*keys)[row];
    for (int col = 0; col < maxCols; ++col) {
      if (col < keyRow.size()) {
        keyButtons[row][col]->setText(keyRow[col]);
        keyButtons[row][col]->setVisible(true);
      } else {
        keyButtons[row][col]->setVisible(false);
      }
    }
  }

  // 隐藏多余的行
  for (; row < maxRows; ++row) {
    for (int col = 0; col < maxCols; ++col)
      keyButtons[row][col]->setVisible(false);
  }
}

QPushButton *VirtualKeyboardWidget::createButton(const QString &text) {
  MyButton *btn = new MyButton(text); // 用自定义按钮替代
  btn->setFixedSize(60, 36);
  btn->setStyleSheet(R"(
    QPushButton {
      background-color: #333;
      border-radius: 6px;
      font-size: 16px;
      color: white;
    }
    QPushButton:pressed {
      background-color: #666;
    }
  )");

  connect(btn, &QPushButton::clicked, this, [=]() {
    const QString keyText = btn->text();
    if (keyText == "←") {
      inputLine->backspace(); // 原"⌫"
    } else if (keyText == "<") {
      inputLine->cursorBackward(false, 1); // 光标左移
    } else if (keyText == ">") {
      inputLine->cursorForward(false, 1); // 光标右移
    } else if (keyText == "空格") {
      inputLine->insert(" ");
    } else if (keyText == "OK") {
      emit textEntered(inputLine->text());
      this->hide();
    } else {
      inputLine->insert(keyText);
    }

    inputLine->setFocus(); // 保证点击按钮后仍保留焦点
  });

  return btn;
}

QString VirtualKeyboardWidget::text() const { return inputLine->text(); }

void VirtualKeyboardWidget::setText(const QString &text) {
  inputLine->setText(text);
}
