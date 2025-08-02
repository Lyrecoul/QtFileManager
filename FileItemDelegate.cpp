#include "FileItemDelegate.h"

FileItemDelegate::FileItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), showEditIcon(false), showDeleteIcon(false) {
}

void FileItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const {
    painter->save();

    QRect rect = option.rect;
    QString name = index.data(Qt::DisplayRole).toString();
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

    // 检查是否是空文件夹提示项（没有图标且没有文本的项）
    bool isEmptyFolderItem = icon.isNull() && name.isEmpty() && index.row() > 0; // row>0 排除面包屑导航

    if (!isEmptyFolderItem) {
        // 背景
        QColor bg = (option.state & QStyle::State_Selected) ? QColor("#333333") : QColor("#2b2b2b");
        painter->setBrush(bg);
        painter->setPen(Qt::NoPen);
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 12, 12);

        // 图标
        QRect iconRect(rect.left() + 14, rect.top() + 10, 22, 22);
        icon.paint(painter, iconRect);

        // 文件名
        QRect textRect = rect.adjusted(44, 0, -28, 0);
        painter->setPen(Qt::white);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

        // 箭头、编辑图标或删除图标
        painter->setPen(QColor("#888888"));
        if (showDeleteIcon) {
          painter->drawText(QRect(rect.right() - 24, rect.top(), 16, rect.height()), Qt::AlignCenter, "✖");
        } else if (showEditIcon) {
          painter->drawText(QRect(rect.right() - 24, rect.top(), 16, rect.height()), Qt::AlignCenter, "✎");
        } else {
          painter->drawText(QRect(rect.right() - 24, rect.top(), 16, rect.height()), Qt::AlignCenter, ">");
        }
    }

    painter->restore();
}

QSize FileItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const {
    return QSize(100, 44);
}

void FileItemDelegate::setShowEditIcon(bool show) {
    showEditIcon = show;
}

bool FileItemDelegate::getShowEditIcon() const {
    return showEditIcon;
}

void FileItemDelegate::setShowDeleteIcon(bool show) {
    showDeleteIcon = show;
}

bool FileItemDelegate::getShowDeleteIcon() const {
    return showDeleteIcon;
}
