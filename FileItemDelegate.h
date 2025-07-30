// FileItemDelegate.h
#ifndef FILEITEMDELEGATE_H
#define FILEITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QIcon>

class FileItemDelegate : public QStyledItemDelegate {
public:
  FileItemDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    painter->save();

    QRect rect = option.rect;
    QString name = index.data(Qt::DisplayRole).toString();
    QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));

    // 背景
    QColor bg = (option.state & QStyle::State_Selected) ? QColor("#444444") : QColor("#2b2b2b");
    painter->setBrush(bg);
    painter->setPen(Qt::NoPen);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawRoundedRect(rect.adjusted(2, 2, -2, -2), 6, 6);

    // 图标
    QRect iconRect(rect.left() + 10, rect.top() + 10, 24, 24);
    icon.paint(painter, iconRect);

    // 文件名
    QRect textRect = rect.adjusted(44, 0, -28, 0);
    painter->setPen(Qt::white);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, name);

    // 箭头
    painter->setPen(QColor("#888888"));
    painter->drawText(QRect(rect.right() - 18, rect.top(), 16, rect.height()), Qt::AlignCenter, ">");

    painter->restore();
  }

  QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override {
    return QSize(100, 44);
  }
};

#endif // FILEITEMDELEGATE_H
