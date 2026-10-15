#pragma once

#include <memory>
#include <type_traits>
#include <utility>

// SimpleFunction：类型擦除的"虚继承流派"实现，与 mini_function（函数指针表流派）对照。
//
// 结构：
//   ICallable          —— 抽象接口：擦除后唯一保留的操作（invoke + clone + 虚析构）
//   Impl<T>            —— 每种 callable 一个派生包装，持有真正的东西
//   SimpleFunction     —— 持有 unique_ptr<ICallable>，一律堆分配
//
// 与 mini_function 的差异：
//   mini_function：函数指针表 + SBO 缓冲（小对象零堆分配、一次间接调用）
//   SimpleFunction：虚函数 + 堆分配（每次构造都 malloc、虚调用两层间接）
//   相同点：非侵入（callable 不需要继承任何类）
template <typename> class SimpleFunction;

template <typename R, typename... Args>
class SimpleFunction<R(Args...)> {
private:
    // 抽象接口：类型擦除的落点
    struct ICallable {
        virtual ~ICallable() = default;
        virtual R invoke(Args... args) const = 0;
        virtual std::unique_ptr<ICallable> clone() const = 0;   // 拷贝入口
    };

    // 每种具体类型 T 一个派生包装
    template <typename T>
    struct Impl : ICallable {
        T callable;

        explicit Impl(T c) : callable(std::move(c)) {}

        R invoke(Args... args) const override {
            return callable(std::forward<Args>(args)...);
        }

        std::unique_ptr<ICallable> clone() const override {
            return std::make_unique<Impl<T>>(callable);
        }
    };

public:
    SimpleFunction() = default;
    SimpleFunction(std::nullptr_t) {}

    // 任意可调用对象：lambda / 函数指针 / 仿函数
    template <typename F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, SimpleFunction>>>
    SimpleFunction(F&& f) {
        static_assert(std::is_copy_constructible_v<std::decay_t<F>>,
                      "SimpleFunction 要求可调用对象可拷贝");
        impl_ = std::make_unique<Impl<std::decay_t<F>>>(std::forward<F>(f));
    }

    // 拷贝：通过虚 clone 深拷贝堆对象
    SimpleFunction(const SimpleFunction& o)
        : impl_(o ? o.impl_->clone() : nullptr) {}
    SimpleFunction& operator=(const SimpleFunction& o) {
        if (this != &o) impl_ = o ? o.impl_->clone() : nullptr;
        return *this;
    }

    SimpleFunction(SimpleFunction&&) noexcept = default;
    SimpleFunction& operator=(SimpleFunction&&) noexcept = default;

    ~SimpleFunction() = default;

    explicit operator bool() const noexcept { return impl_ != nullptr; }

    R operator()(Args... args) const {
        return impl_->invoke(std::forward<Args>(args)...);
    }

private:
    std::unique_ptr<ICallable> impl_;
};
