#pragma once

#include <cstddef>
#include <cstring>
#include <utility>

// MoveBuffer：独占堆缓冲，完整演示"拷贝 vs 移动"的内存路径。
//
// 内存模型：
//   对象 = size_ + cap_ + data_（24 字节）
//   拷贝 → 堆上新分配一份数据（两份数据，源不动）
//   移动 → 只搬 data_ 指针并置空源（零分配，O(1)）
//   noexcept 移动 → std::vector 扩容时用移动而非拷贝

class MoveBuffer {
public:
    explicit MoveBuffer(std::size_t n = 0)
        : size_(n), cap_(n), data_(n ? new char[n] : nullptr) {}

    ~MoveBuffer() { delete[] data_; }

    MoveBuffer(const MoveBuffer& o)
        : size_(o.size_), cap_(o.cap_),
          data_(o.size_ ? new char[o.size_] : nullptr) {
        if (data_) {
            std::memcpy(data_, o.data_, size_);
        }
    }

    MoveBuffer& operator=(const MoveBuffer& o) {
        if (this != &o) {
            MoveBuffer tmp(o);   // copy-and-swap：异常安全
            swap(tmp);
        }
        return *this;
    }

    // 移动构造：搬指针 + 置空源。noexcept 是关键——见 README 原理节
    MoveBuffer(MoveBuffer&& o) noexcept
        : size_(o.size_), cap_(o.cap_), data_(o.data_) {
        o.size_ = 0;
        o.cap_ = 0;
        o.data_ = nullptr;
    }

    MoveBuffer& operator=(MoveBuffer&& o) noexcept {
        if (this != &o) {
            delete[] data_;
            size_ = o.size_;
            cap_ = o.cap_;
            data_ = o.data_;
            o.size_ = 0;
            o.cap_ = 0;
            o.data_ = nullptr;
        }
        return *this;
    }

    void swap(MoveBuffer& o) noexcept {
        using std::swap;
        swap(size_, o.size_);
        swap(cap_, o.cap_);
        swap(data_, o.data_);
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return cap_; }
    char* data() const { return data_; }

private:
    std::size_t size_ = 0;
    std::size_t cap_ = 0;
    char* data_ = nullptr;
};
