#pragma once

#include <memory>

class QString;

// PImpl（Pointer to Implementation）：
// 头文件只暴露接口 + 指向实现的指针，实现类完整定义藏在 .cpp。
// 收益：编译隔离（改实现不触发依赖方重编译）、ABI 稳定（对象大小固定）、隐藏实现。
// Qt 的 d-pointer（Q_DECLARE_PRIVATE / QObject::d_ptr）就是 PImpl 的框架内形态。
class MediaPlayer {
public:
    MediaPlayer();
    // unique_ptr<Impl> 的析构需要 Impl 完整类型，所以析构必须在 .cpp 定义
    ~MediaPlayer();

    MediaPlayer(const MediaPlayer&);            // Impl 深拷贝（完整类型只在 .cpp）
    MediaPlayer& operator=(const MediaPlayer&);

    MediaPlayer(MediaPlayer&&) noexcept;
    MediaPlayer& operator=(MediaPlayer&&) noexcept;

    void open(const QString& path);
    void play();
    void pause();
    double volume() const;
    void setVolume(double volume);

private:
    struct Impl;                 // 前向声明：只告诉编译器"存在一个 Impl"
    std::unique_ptr<Impl> d_;    // d-pointer：指向实现，接口层只知道有指针
};
