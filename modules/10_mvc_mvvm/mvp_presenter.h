#pragma once

#include <QString>
#include <QStringList>

// MVP：数据回流路径上没有信号——Presenter 每次操作完主动调 View 接口。
// 纯 C++（无 Q_OBJECT）：单测传一个 FakeView 即可，不依赖 Qt 信号/GUI。

// 被动视图接口：View 不订阅任何数据源，只等 Presenter 主动驱动
class IPlaylistView {
public:
    virtual ~IPlaylistView() = default;
    virtual void setTitles(const QStringList& titles) = 0;
    virtual void setCurrent(const QString& title) = 0;
};

class PlaylistPresenter {
public:
    explicit PlaylistPresenter(IPlaylistView* view);

    void init();          // 初始画面也由 Presenter 推送
    void onAddTitle();    // 事件入口：View 把按钮点击转交过来
    void onNext();

private:
    IPlaylistView* view_;
    QStringList items_;
    int current_ = -1;
};
