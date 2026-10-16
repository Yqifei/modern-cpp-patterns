#include "view_model.h"

PlaylistViewModel::PlaylistViewModel(QObject* parent)
    : QObject(parent) {}

QString PlaylistViewModel::currentTitle() const {
    return current_ >= 0 ? items_.at(current_) : QString();
}

QStringList PlaylistViewModel::titles() const {
    return items_;
}

void PlaylistViewModel::addTrack() {
    items_ << QString("曲目%1").arg(items_.size() + 1);
    emit titlesChanged(items_);    // 列表变了：绑定方自己刷新
    if (current_ < 0) {            // 首曲自动成为当前（三页契约一致）
        current_ = 0;
        emit currentTitleChanged(items_.first());
    }
}

void PlaylistViewModel::next() {
    if (items_.isEmpty()) {
        return;
    }
    current_ = (current_ + 1) % items_.size();
    emit currentTitleChanged(items_.at(current_));
}
