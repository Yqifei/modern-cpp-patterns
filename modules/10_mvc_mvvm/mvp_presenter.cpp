#include "mvp_presenter.h"

PlaylistPresenter::PlaylistPresenter(IPlaylistView* view)
    : view_(view) {}

void PlaylistPresenter::init() {
    view_->setTitles(items_);      // 连初始画面都来自 Presenter
    view_->setCurrent(QString());
}

void PlaylistPresenter::onAddTitle() {
    items_ << QString("曲目%1").arg(items_.size() + 1);
    view_->setTitles(items_);      // 每次操作完主动驱动 View
    if (current_ < 0) {            // 首曲自动成为当前（三页契约一致）
        current_ = 0;
        view_->setCurrent(items_.first());
    }
}

void PlaylistPresenter::onNext() {
    if (items_.isEmpty()) {
        return;
    }
    current_ = (current_ + 1) % items_.size();
    view_->setCurrent(items_.at(current_));
}
