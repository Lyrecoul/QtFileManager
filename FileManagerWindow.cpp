#include "FileManagerWindow.h"
#include "ImageViewer.h"
#include "qglobal.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QIcon>
#include <QSizePolicy>
#include <QScroller>
#include <QScrollerProperties>
#include <QEasingCurve>
#include <QDebug>
#include <QHeaderView>
#include <QScrollBar>
#include <QDateTime>
#include <QCoreApplication>
#include <QLabel>

FileManagerWindow::FileManagerWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_DeleteOnClose, false);

    // 创建文件系统模型
    model = new QFileSystemModel(this);
    model->setRootPath(rootPath);
    model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    // 创建树视图
    tree = new QTreeView(this);
    tree->setModel(model);
    tree->setRootIndex(model->index(rootPath));
    tree->setHeaderHidden(true);
    tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tree->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    tree->setFocusPolicy(Qt::StrongFocus);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->setIconSize(QSize(14, 14));  // 增大图标
    tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); // 滚动更细腻

    // 禁用展开/折叠功能
    tree->setItemsExpandable(false);
    tree->setRootIsDecorated(false); // 不显示展开箭头

    // 启用更平滑的触摸滑动
    QScroller *scroller = QScroller::scroller(tree->viewport());
    QScrollerProperties sp = scroller->scrollerProperties();

    // 进一步优化触控参数
    sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.0005);
    sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.05);
    sp.setScrollMetric(QScrollerProperties::DecelerationFactor, 0.05);
    sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 2.0);
    sp.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.5);
    sp.setScrollMetric(QScrollerProperties::FrameRate, QScrollerProperties::Fps60);
    sp.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.0);
    sp.setScrollMetric(QScrollerProperties::ScrollingCurve, QEasingCurve(QEasingCurve::Linear)); // 线性，无缓动
    sp.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 1.0); // 基本无回弹动画
    sp.setScrollMetric(QScrollerProperties::OvershootScrollTime, 0.01); // 回弹动画极快
    sp.setScrollMetric(QScrollerProperties::SnapTime, 0.01); // 吸附动画极快

    scroller->setScrollerProperties(sp);
    scroller->grabGesture(tree->viewport(), QScroller::TouchGesture); // 支持触摸滑动
    scroller->grabGesture(tree->viewport(), QScroller::LeftMouseButtonGesture);

    // 长按支持 (右键菜单备用)
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);

    // 更完善的暗黑风格样式 + 微软雅黑字体
    setStyleSheet(R"(
        QWidget {
            background-color: #121212;
            color: #f0f0f0;
            font-size: 14px;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QTreeView {
            background-color: #1e1e1e;
            alternate-background-color: #2a2a2a;
            color: #e0e0e0;
            border: none;
            outline: none;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QTreeView::branch {
            background: none;
            border: none;
            image: none;
            width: 0px;
        }
        QTreeView::item {
            padding: 12px 8px;
            height: 14px;
            margin: 2px 0;
        }
        QTreeView::item:hover {
            background-color: #2a2a2a;
        }
        QTreeView::item:selected {
            background-color: #555555;
            color: #ffffff;
        }
        QPushButton {
            background-color: transparent;
            border: none;
            padding: 4px;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QPushButton:hover {
            background-color: #333333;
            border-radius: 4px;
        }
        QLabel {
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        /* 自定义滚动条样式 */
        QScrollBar:vertical {
            background: #1e1e1e;
            width: 10px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            min-height: 20px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #777777;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: none;
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )");

    // 创建按钮
    backButton = new QPushButton(this);
    backButton->setIcon(QIcon(":/icons/back.png"));
    backButton->setIconSize(QSize(20, 20));
    backButton->setFixedSize(32, 32);
    backButton->setFlat(true);

    QPushButton *closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/icons/close.png"));
    closeButton->setIconSize(QSize(20, 20));
    closeButton->setFixedSize(32, 32);
    closeButton->setFlat(true);
    closeButton->setToolTip("隐藏");

    // 路径标签
    pathLabel = new QLabel(this);
    pathLabel->setText(rootPath);
    pathLabel->setStyleSheet("color: #bbbbbb; font-size: 13px; padding: 0 8px;");
    pathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pathLabel->setMaximumWidth(320); // 限制最大宽度
    pathLabel->setMinimumWidth(40);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    pathLabel->setWordWrap(false);

    // 修改后的布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);      // 后退按钮靠左
    buttonLayout->addWidget(pathLabel);       // 路径标签在中间
    buttonLayout->addStretch();               // 添加弹性空间
    buttonLayout->addWidget(closeButton);     // 关闭按钮靠右
    buttonLayout->setSpacing(4);
    buttonLayout->setContentsMargins(6, 6, 6, 6);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(tree);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);

    // 连接信号
    connect(backButton, &QPushButton::clicked, this, &FileManagerWindow::goBack);
    connect(closeButton, &QPushButton::clicked, this, &FileManagerWindow::close);
    connect(tree, &QTreeView::clicked, this, &FileManagerWindow::onFileClicked);
    connect(tree, &QTreeView::doubleClicked, this, &FileManagerWindow::onFileClicked);
}

void FileManagerWindow::goBack()
{
    QString currentPath = model->filePath(tree->rootIndex());
    QString parentPath = QFileInfo(currentPath).dir().absolutePath();
    if (parentPath.startsWith(rootPath) && parentPath != currentPath) {
        tree->setRootIndex(model->index(parentPath));
        pathLabel->setText(QFileInfo(parentPath).absoluteFilePath());
        pathLabel->setToolTip(parentPath); // 鼠标悬停显示完整路径
    }
}

void FileManagerWindow::onFileClicked(const QModelIndex &index)
{
    static qint64 lastClickTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    // 防误触判断 (300ms 内不处理二次点击)
    if(currentTime - lastClickTime < 300) return;
    lastClickTime = currentTime;

    if (!index.isValid()) return;

    QString path = model->filePath(index);
    QFileInfo fileInfo(path);

    if (fileInfo.isDir()) {
        if (path.startsWith(rootPath)) {
            tree->setRootIndex(model->index(path));
            pathLabel->setText(QFileInfo(path).absoluteFilePath());
            pathLabel->setToolTip(path); // 鼠标悬停显示完整路径
        }
        return;
    }

    QMimeDatabase db;
    QString mime = db.mimeTypeForFile(fileInfo).name();

    if (mime.startsWith("image/")) {
        auto *viewer = new ImageViewer(path, this);
        viewer->showFullScreen();
    } else if (mime.startsWith("video/")) {
        // 获取程序所在目录并启动 Video 程序
        QString program = QCoreApplication::applicationDirPath() + "/ffmpeg/Video";  // 获取当前程序路径，并加上 video 程序名
        QStringList arguments;
        arguments << path;  // 传递视频文件路径作为参数

        // 启动视频程序
        QProcess::startDetached(program, arguments);
    } else if (fileInfo.isExecutable()) {
        QProcess::startDetached(path, QStringList());
    }
}
