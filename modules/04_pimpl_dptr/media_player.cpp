#include "media_player.h"

#include <QDebug>
#include <QString>

// Impl 完整定义只在 .cpp —— 外部看不到它的任何成员。
// 增加/删除 Impl 成员不影响头文件，依赖方无需重新编译。
struct MediaPlayer::Impl {
    QString path;
    bool playing = false;
    double volume = 0.8;
    // 私有状态可以随意扩展：缓存、解码器句柄、统计……全部对外隐藏

    void open(const QString& p) {
        path = p;
        qDebug().noquote() << "  打开媒体:" << path;
    }

    void play() {
        playing = true;
        qDebug().noquote() << "  播放:" << path << "音量" << volume;
    }

    void pause() {
        playing = false;
        qDebug().noquote() << "  暂停:" << path;
    }
};

MediaPlayer::MediaPlayer() : d_(std::make_unique<Impl>()) {}

// 析构在这里定义：此刻 Impl 完整可见，unique_ptr 才能正确销毁它
MediaPlayer::~MediaPlayer() = default;

MediaPlayer::MediaPlayer(const MediaPlayer& o)
    : d_(std::make_unique<Impl>(*o.d_)) {}

MediaPlayer& MediaPlayer::operator=(const MediaPlayer& o) {
    if (this != &o) d_ = std::make_unique<Impl>(*o.d_);
    return *this;
}

MediaPlayer::MediaPlayer(MediaPlayer&&) noexcept = default;
MediaPlayer& MediaPlayer::operator=(MediaPlayer&&) noexcept = default;

void MediaPlayer::open(const QString& path) { d_->open(path); }
void MediaPlayer::play() { d_->play(); }
void MediaPlayer::pause() { d_->pause(); }
double MediaPlayer::volume() const { return d_->volume; }
void MediaPlayer::setVolume(double volume) { d_->volume = volume; }
