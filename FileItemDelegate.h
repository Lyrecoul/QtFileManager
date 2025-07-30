// FileItemDelegate.h
#ifndef FILEITEMDELEGATE_H
#define FILEITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QIcon>

class FileItemDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit FileItemDelegate(QObject *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

  void setShowEditIcon(bool show);
  bool getShowEditIcon() const;

private:
  bool showEditIcon;
};

#endif // FILEITEMDELEGATE_H
