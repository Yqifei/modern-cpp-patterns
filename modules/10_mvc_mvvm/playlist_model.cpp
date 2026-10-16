#include "playlist_model.h"

PlaylistModel::PlaylistModel(QObject* parent)
    : QAbstractListModel(parent) {}

int PlaylistModel::rowCount(const QModelIndex& parent) const {
    // 列表模型：只有根层有行
    return parent.isValid() ? 0 : items_.size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= items_.size()) {
        return {};
    }
    return role == Qt::DisplayRole ? items_.at(index.row()) : QVariant();
}

void PlaylistModel::addTitle() {
    const int row = items_.size();
    beginInsertRows(QModelIndex(), row, row);      // 通知框架"要插行了"
    items_.append(QString("曲目%1").arg(row + 1));
    endInsertRows();                               // 框架发 rowsInserted → QListView 自动刷新
    if (current_ < 0) {                            // 首曲自动成为当前（三页契约一致）
        current_ = 0;
        emit currentChanged(items_.first());
    }
}

void PlaylistModel::next() {
    if (items_.isEmpty()) {
        return;
    }
    current_ = (current_ + 1) % items_.size();
    emit currentChanged(items_.at(current_));
}

QString PlaylistModel::currentTitle() const {
    return current_ >= 0 ? items_.at(current_) : QString();
}
