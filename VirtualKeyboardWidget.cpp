#include "VirtualKeyboardWidget.h"
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QScroller>

VirtualKeyboardWidget::VirtualKeyboardWidget(QWidget *parent)
    : QWidget(parent), currentPage(Letters) {
  initializeKeyboard("", "");
}

VirtualKeyboardWidget::VirtualKeyboardWidget(const QString &defaultText,
                                             const QString &placeholderText,
                                             QWidget *parent)
    : QWidget(parent), currentPage(Letters) {
  initializeKeyboard(defaultText, placeholderText);
}

void VirtualKeyboardWidget::initializeKeyboard(const QString &defaultText,
                                               const QString &placeholderText) {
  isUpperCase = false; // 默认小写模式
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
      "QLineEdit { font-size: 16px; background-color: #222; color: white; padding: 4px; border: none; border-radius: 6px; }"
      "QLineEdit:focus { border: none; outline: none; }");
  inputLine->setReadOnly(false); // 让游标显示
  inputLine->setFocus();

  // 设置默认文本和占位符
  inputLine->setText(defaultText);
  inputLine->setPlaceholderText(placeholderText);

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
  topLayout->setSpacing(5); // 输入框和按钮之间的间距

  // Tab 分组按钮
  QWidget *tabWidget = new QWidget(this);
  tabWidget->setFixedHeight(40); // 强制高度，防止下面出现空白
  tabWidget->setStyleSheet("background-color: #111;");

  QHBoxLayout *tabLayout = new QHBoxLayout(tabWidget);
  tabLayout->setContentsMargins(4, 0, 4, 0);
  tabLayout->setSpacing(4);

  QStringList tabs = {"字母", "数字", "符号"};

  // 创建大小写切换按钮
  caseToggleBtn = new QPushButton("⇧");
  caseToggleBtn->setCheckable(true);
  caseToggleBtn->setStyleSheet(R"(
    QPushButton {
      background-color: #222; color: white;
      padding: 6px; border-radius: 6px;
      font-weight: bold;
    }
    QPushButton:checked {
      background-color: #555;
    })");

  connect(caseToggleBtn, &QPushButton::clicked, this,
          &VirtualKeyboardWidget::toggleCase);
  tabLayout->addWidget(caseToggleBtn);

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
  keyLayout->setContentsMargins(4, 0, 4, 4); // 减小顶部边距，消除间隙
  keyLayout->setSpacing(4);

  scrollArea->setWidget(keyboardWidget);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->addLayout(topLayout);
  mainLayout->addWidget(tabWidget);
  mainLayout->addWidget(scrollArea);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0); // 消除组件之间的间距
  mainLayout->setStretch(2, 1); // 让键盘区域占据剩余空间
  setLayout(mainLayout);

  // 初始化按键内容
  letterKeys = {{"←", "空格", "<", ">", "清空"},
                {"a", "b", "c", "d", "e"},
                {"f", "g", "h", "i", "j"},
                {"k", "l", "m", "n", "o"},
                {"p", "q", "r", "s", "t"},
                {"u", "v", "w", "x", "y"},
                {"z"}};

  letterKeysUpper = {{"←", "空格", "<", ">", "清空"},
                     {"A", "B", "C", "D", "E"},
                     {"F", "G", "H", "I", "J"},
                     {"K", "L", "M", "N", "O"},
                     {"P", "Q", "R", "S", "T"},
                     {"U", "V", "W", "X", "Y"},
                     {"Z"}};

  numberKeys = {{"←", "空格", "<", ">", "清空"},
                {"1", "2", "3", "4", "5"},
                {"6", "7", "8", "9", "0"},
                {"+", "-", "*", "/", "="},
                {"OK"}};

  symbolKeys = {
      {"←", "空格", "<", ">", "清空"},
      {"!", "@", "#", "$", "%"},
      {"^", "&", "*", "(", ")"},
      {"[", "]", "{", "}", "\\"},
  };

  QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);
  scrollArea->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);
  scrollArea->viewport()->setAttribute(Qt::WA_TransparentForMouseEvents, false);

  for (int row = 0; row < maxRows; ++row) {
    QHBoxLayout *rowLayout = new QHBoxLayout;
    rowLayout->setSpacing(4); // 确保按钮之间有适当的间距
    QVector<QPushButton *> rowButtons;
    for (int col = 0; col < maxCols; ++col) {
      QPushButton *btn = createButton(""); // 空按钮
      btn->setVisible(false);              // 初始不可见
      rowLayout->addWidget(btn);
      rowButtons.append(btn);
    }
    // 设置行布局的拉伸因子，使按钮能够拉伸填充空间
    for (int col = 0; col < maxCols; ++col) {
      rowLayout->setStretch(col, 1);
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
    keys = isUpperCase ? &letterKeysUpper : &letterKeys;
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
  btn->setMinimumSize(60, 36); // 改为最小尺寸，允许按钮拉伸
  btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // 设置大小策略为可扩展
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
    } else if (keyText == "清空") {
      inputLine->clear();
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

void VirtualKeyboardWidget::setPlaceholderText(const QString &text) {
  inputLine->setPlaceholderText(text);
}

void VirtualKeyboardWidget::toggleCase() {
  isUpperCase = !isUpperCase;
  caseToggleBtn->setChecked(isUpperCase);
  // 如果当前在字母页面，立即更新键盘显示
  if (currentPage == Letters) {
    buildKeyboard();
  }
}
