#pragma once

#include <algorithm>
#include <cstddef>
#include <utility>

// Copy-and-Swap：拷贝赋值运算符按值传参 + swap 实现。
//   FrameBuffer& operator=(FrameBuffer o) noexcept { swap(o); return *this; }
//
// 为什么这么写：
//   ① 拷贝在"参数构造"时完成——若拷贝抛异常，this 未被触碰（强异常安全）；
//   ② swap 不抛异常，交换后参数持有旧资源，其析构负责释放；
//   ③ 自赋值天然安全（先拷贝再交换，没有"先析构自己再拷贝"的悬空风险）；
//   ④ 传右值时参数走移动构造，同一个函数同时服务拷贝赋值和移动赋值。

class FrameBuffer {
public:
    explicit FrameBuffer(std::size_t size = 0)
        : data_(size ? new char[size] : nullptr), size_(size) {}

    ~FrameBuffer() { delete[] data_; }

    FrameBuffer(const FrameBuffer& o)
        : data_(o.size_ ? new char[o.size_] : nullptr), size_(o.size_) {
        if (o.size_) std::copy(o.data_, o.data_ + o.size_, data_);
    }

    // Copy-and-Swap：按值传参（拷贝或移动都在这里完成）
    FrameBuffer& operator=(FrameBuffer o) noexcept {
        swap(o);
        return *this;
    }

    FrameBuffer(FrameBuffer&& o) noexcept : data_(nullptr), size_(0) {
        swap(o);   // 默认资源 + swap：把 o 的内容搬过来，o 变空
    }

    FrameBuffer& operator=(FrameBuffer&& o) noexcept {
        if (this != &o) swap(o);
        return *this;
    }

    void swap(FrameBuffer& o) noexcept {
        std::swap(data_, o.data_);
        std::swap(size_, o.size_);
    }

    std::size_t size() const { return size_; }
    const char* data() const { return data_; }

private:
    char* data_;
    std::size_t size_;
};
