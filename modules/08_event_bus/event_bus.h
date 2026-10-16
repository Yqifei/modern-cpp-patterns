#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

// EventBus：集中式发布-订阅
//
// 观察者是一对多直连（发布者持有订阅者列表）；
// EventBus 是任意对任意——发布者不认识订阅者，只 publish 事件类型。
//
// 设计决策：
//   - 按事件类型（type_index）路由，subscribe<E>/publish<E> 类型安全
//   - 回调统一擦除为 void(const void*)，内部再还原为 const E&
//   - 发布时先拷贝订阅快照再执行回调：回调里可再 publish / 退订，不死锁不失效
//   - subscribeWeak 用 weak_ptr 托住订阅者：销毁后自动跳过（防悬空）

class EventBus {
public:
    using Handler = std::function<void(const void*)>;

    // 强订阅：持有回调直到退订
    template <typename E, typename F>
    std::size_t subscribe(F&& f) {
        Handler h = [f = std::forward<F>(f)](const void* e) mutable {
            f(*static_cast<const E*>(e));
        };
        return add(std::type_index(typeid(E)), std::move(h));
    }

    // 弱订阅：订阅者对象销毁后自动跳过（guard 提升失败即跳过）
    // 参数是 weak_ptr（lock() 是 weak_ptr 的成员，返回 shared_ptr 临时提升所有权；
    // shared_ptr 没有 lock()——它本身就持有所有权）
    template <typename E, typename G, typename F>
    std::size_t subscribeWeak(const std::weak_ptr<G>& guard, F&& f) {
        Handler h = [guard, f = std::forward<F>(f)](const void* e) mutable {
            if (auto sp = guard.lock()) {
                f(*static_cast<const E*>(e));
            }
        };
        return add(std::type_index(typeid(E)), std::move(h));
    }

    template <typename E>
    void publish(const E& event) {
        std::vector<Entry> snapshot;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            auto it = table_.find(std::type_index(typeid(E)));
            if (it == table_.end()) {
                return;
            }
            snapshot = it->second;      // 拷贝快照：锁外执行回调
        }
        for (const auto& entry : snapshot) {
            entry.handler(&event);
        }
    }

    void unsubscribe(std::size_t token) {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& kv : table_) {
            auto& handlers = kv.second;
            handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
                                          [token](const Entry& e) {
                                              return e.token == token;
                                          }),
                           handlers.end());
        }
    }

    template <typename E>
    std::size_t subscriberCount() const {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = table_.find(std::type_index(typeid(E)));
        return it == table_.end() ? 0 : it->second.size();
    }

private:
    struct Entry {
        std::size_t token = 0;
        Handler handler;
    };

    std::size_t add(std::type_index type, Handler h) {
        std::lock_guard<std::mutex> lk(mtx_);
        table_[type].push_back({++nextToken_, std::move(h)});
        return nextToken_;
    }

    mutable std::mutex mtx_;
    std::unordered_map<std::type_index, std::vector<Entry>> table_;
    std::size_t nextToken_ = 0;
};
