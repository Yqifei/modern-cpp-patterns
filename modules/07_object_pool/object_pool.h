#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

// 对象池：复用实例，避免反复构造/析构。
//
// 语义：
//   acquire() 借出 —— 有空闲实例就复用，池空才新建
//   release() 归还 —— 实例回到空闲列表，等待下次借用
//   所有权用 unique_ptr 表达：借出期间调用方持有，归还即交回池
//
// 与工厂的区别：工厂每次创建新对象；池优先复用，新建只是兜底。

template <typename T>
class ObjectPool {
public:
    using Creator = std::function<std::unique_ptr<T>()>;

    explicit ObjectPool(Creator creator, std::size_t reserve = 0)
        : creator_(std::move(creator)) {
        for (std::size_t i = 0; i < reserve; ++i) {
            idle_.push_back(creator_());
        }
    }

    std::unique_ptr<T> acquire() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idle_.empty()) {
            return creator_();      // 池空：新建兜底
        }
        auto obj = std::move(idle_.back());
        idle_.pop_back();
        return obj;
    }

    void release(std::unique_ptr<T> obj) {
        std::lock_guard<std::mutex> lk(mtx_);
        idle_.push_back(std::move(obj));
    }

    std::size_t idleCount() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return idle_.size();
    }

private:
    mutable std::mutex mtx_;
    Creator creator_;
    std::vector<std::unique_ptr<T>> idle_;
};
