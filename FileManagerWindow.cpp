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
#include <QStyledItemDelegate>

// 自定义文件系统模型，首列显示文字图标和文件大小
class TextIconFileSystemModel : public QFileSystemModel {
public:
    using QFileSystemModel::QFileSystemModel;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && index.column() == 0) {
            QFileInfo info = QFileSystemModel::fileInfo(index);
            QString icon;
            if (info.isDir())
                icon = QStringLiteral(" 📁 ");
            else if (info.isExecutable())
                icon = QStringLiteral(" 🔧 ");
            else if (QStringList{"png","jpg","jpeg","bmp","gif"}.contains(info.suffix().toLower()))
                icon = QStringLiteral(" 🎨 ");
            else if (QStringList{"mp4","avi","mkv","mov"}.contains(info.suffix().toLower()))
                icon = QStringLiteral(" 🎬 ");
            else
                icon = QStringLiteral(" 📄 ");
            QString name = QFileSystemModel::data(index, role).toString();

            // 文件大小
            QString sizeStr;
            if (info.isDir()) {
                sizeStr = "-";
            } else {
                qint64 size = info.size();
                if (size < 1024)
                    sizeStr = QString::number(size) + " B";
                else if (size < 1024 * 1024)
                    sizeStr = QString::number(size / 1024.0, 'f', 1) + " KB";
                else if (size < 1024 * 1024 * 1024)
                    sizeStr = QString::number(size / 1024.0 / 1024.0, 'f', 1) + " MB";
                else
                    sizeStr = QString::number(size / 1024.0 / 1024.0 / 1024.0, 'f', 1) + " GB";
            }
            // 右对齐大小，使用空格填充
            int totalWidth = 36; // 总宽度（可根据字体调整）
            QString display = icon + name;
            int pad = totalWidth - display.length() - sizeStr.length();
            if (pad < 2) pad = 2;
            display += QString(pad, QChar(' ')) + sizeStr;
            return display;
        }
        // 不返回 DecorationRole，避免原生图标
        if (role == Qt::DecorationRole && index.column() == 0) {
            return QVariant();
        }
        return QFileSystemModel::data(index, role);
    }
};

namespace {
    QString formatDisplayPath(const QString &path) {
        QString result = path;
        if (result.startsWith("/userdisk/Music")) {
            result.replace(0, QString("/userdisk/Music").length(), QStringLiteral("存储"));
        }
        return result;
    }
}

FileManagerWindow::FileManagerWindow(QWidget *parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_DeleteOnClose, false);

    // 固定窗口大小，适配 320x170
    setFixedSize(320, 170);

    // 创建自定义文件系统模型
    model = new TextIconFileSystemModel(this);
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
    tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // 只显示首列
    for (int i = 1; i < model->columnCount(); ++i) {
        tree->setColumnHidden(i, true);
    }

    // 禁用展开/折叠功能
    tree->setItemsExpandable(false);
    tree->setRootIsDecorated(false);

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
    scroller->grabGesture(tree->viewport(), QScroller::TouchGesture);
    scroller->grabGesture(tree->viewport(), QScroller::LeftMouseButtonGesture);

    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->viewport()->setAttribute(Qt::WA_AcceptTouchEvents);

    // One Dark 风格样式，适合小屏
    setStyleSheet(R"(
        QWidget {
            background-color: #282c34;
            color: #abb2bf;
            font-size: 12px;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QTreeView {
            background-color: #21252b;
            alternate-background-color: #23252b;
            color: #abb2bf;
            border: none;
            outline: none;
            font-size: 12px;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QTreeView::item {
            padding: 4px 4px;
            height: 16px;
            margin: 1px 0;
            border-radius: 4px;
        }
        QTreeView::item:hover {
            background-color: #3e4451;
        }
        QTreeView::item:selected {
            background-color: #61afef;
            color: #282c34;
        }
        QPushButton {
            background-color: transparent;
            border: none;
            padding: 2px;
            font-size: 12px;
            color: #abb2bf;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QPushButton:hover {
            background-color: #3e4451;
            border-radius: 4px;
        }
        QLabel {
            font-size: 11px;
            color: #5c6370;
            font-family: "Microsoft YaHei", "微软雅黑", "Arial", sans-serif;
        }
        QScrollBar:vertical {
            background: #21252b;
            width: 8px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #3e4451;
            min-height: 16px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical:hover {
            background: #61afef;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: none;
            height: 0px;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )");

    // 按钮尺寸适配小屏
    backButton = new QPushButton(this);
    backButton->setIcon(QIcon(":/icons/back.png"));
    backButton->setIconSize(QSize(20, 20)); // 稍大
    backButton->setFixedSize(30, 30);       // 稍大
    backButton->setFlat(true);

    QPushButton *closeButton = new QPushButton(this);
    closeButton->setIcon(QIcon(":/icons/close.png"));
    closeButton->setIconSize(QSize(20, 20)); // 稍大
    closeButton->setFixedSize(30, 30);       // 稍大
    closeButton->setFlat(true);
    closeButton->setToolTip("隐藏");

    // 路径标签
    pathLabel = new QLabel(this);
    pathLabel->setText(formatDisplayPath(rootPath));
    pathLabel->setStyleSheet("color: #b0bec5; font-size: 11px; padding: 0 8px;"); // padding略增
    pathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pathLabel->setMaximumWidth(220); // 最大宽度增加
    pathLabel->setMinimumWidth(20);
    pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    pathLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    pathLabel->setWordWrap(false);

    // 更宽松的布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addSpacing(8); // 按钮与路径标签间距
    buttonLayout->addWidget(pathLabel);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    buttonLayout->setSpacing(8); // 按钮之间间距
    buttonLayout->setContentsMargins(6, 3, 6, 3); // 边距加大

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(tree);
    mainLayout->setSpacing(6); // 主体间距加大
    mainLayout->setContentsMargins(0, 2, 0, 2); // 主体边距加大
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
        QString displayParentPath = QFileInfo(parentPath).absoluteFilePath();
        displayParentPath = formatDisplayPath(displayParentPath);
        pathLabel->setText(displayParentPath);
        pathLabel->setToolTip(displayParentPath); // 鼠标悬停显示完整路径
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
            QString displayPath = QFileInfo(path).absoluteFilePath();
            displayPath = formatDisplayPath(displayPath);
            pathLabel->setText(displayPath);
            pathLabel->setToolTip(displayPath); // 鼠标悬停显示完整路径
        }
        return;
    }

    QMimeDatabase db;
    QString mime = db.mimeTypeForFile(fileInfo).name();

    if (mime.startsWith("image/")) {
        auto *viewer = new ImageViewer(path, this);
        viewer->showFullScreen();
    } else if (mime.startsWith("video/") || mime.startsWith("audio/")) {
        // 视频和音频都交由 VideoPlayer 处理
        QString program = QCoreApplication::applicationDirPath() + "/VideoPlayer";
        QStringList arguments;
        arguments << path;
        QProcess::startDetached(program, arguments);
    } else if (fileInfo.isExecutable()) {
        QProcess::startDetached(path, QStringList());
    }
}
