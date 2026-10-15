#pragma once

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

// mini_function：std::function 的最小实现，类型擦除的完整骨架。
//
// 内存模型：
//   每个对象持有 操作表指针(vt_) + 数据指针(data_) + 16 字节 SBO 缓冲。
//   - callable 够小（<=16B、对齐满足、nothrow 移动）→ 放对象内缓冲，零堆分配
//   - 太大 → 放堆
//   调用 = 一次函数指针间接跳转。
//
// 与 libstdc++ 的差异（取舍）：
//   ① SBO 阈值固定 2 个指针，未做动态最适值
//   ② 未处理非平凡移动但不可拷贝的极端 callable 类型
template <typename> class mini_function;

template <typename R, typename... Args>
class mini_function<R(Args...)> {
public:
    using result_type = R;

    mini_function() noexcept = default;
    mini_function(std::nullptr_t) noexcept {}

    // 任意可调用对象：lambda / 函数指针 / 仿函数 / bind 结果
    template <typename F,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<F>, mini_function>>>
    mini_function(F&& f) { assign(std::forward<F>(f)); }

    mini_function(const mini_function& o) { copy_from(o); }
    mini_function& operator=(const mini_function& o) {
        if (this != &o) { reset(); copy_from(o); }
        return *this;
    }

    mini_function(mini_function&& o) noexcept { move_from(o); }
    mini_function& operator=(mini_function&& o) noexcept {
        if (this != &o) { reset(); move_from(o); }
        return *this;
    }

    ~mini_function() { reset(); }

    explicit operator bool() const noexcept { return vt_ != nullptr; }

    R operator()(Args... args) const {
        return vt_->invoke(data_, std::forward<Args>(args)...);
    }

private:
    // 类型擦除的落点：每种 T 实例化一组静态操作，运行时统一走函数指针
    struct vtable {
        void (*destroy)(void*);               // 仅析构
        void (*move_construct)(void*, void*); // placement new 搬运
        void (*copy_construct)(const void*, void*); // placement new 拷贝
        void* (*clone)(const void*);          // 堆拷贝（新分配副本）
        void (*deallocate)(void*);            // 释放内存（与 new T(...) 对称）
        R    (*invoke)(void*, Args...);       // 调用
    };

    static constexpr std::size_t kSboBytes = 2 * sizeof(void*);
    alignas(std::max_align_t) unsigned char sbo_[kSboBytes];
    void* data_ = nullptr;        // SBO 时 == sbo_；堆时 == 堆指针
    const vtable* vt_ = nullptr;

    // SBO 判定：尺寸 / 对齐 / 移动不抛（保证 move 安全）
    template <typename F>
    static constexpr bool fits_sbo =
        sizeof(F) <= kSboBytes &&
        alignof(F) <= alignof(std::max_align_t) &&
        std::is_nothrow_move_constructible_v<F>;

    template <typename F>
    static void destroy_op(void* p) { static_cast<F*>(p)->~F(); }

    template <typename F>
    static void move_op(void* from, void* to) {
        ::new (to) F(std::move(*static_cast<F*>(from)));
        static_cast<F*>(from)->~F();
    }

    template <typename F>
    static void copy_op(const void* from, void* to) {
        ::new (to) F(*static_cast<const F*>(from));
    }

    template <typename F>
    static void* clone_op(const void* p) {
        return new F(*static_cast<const F*>(p));
    }

    static void deallocate_op(void* p) { ::operator delete(p); }

    template <typename F>
    static R invoke_op(void* p, Args... args) {
        return (*static_cast<F*>(p))(std::forward<Args>(args)...);
    }

    template <typename F>
    static const vtable* get_vtable() {
        static const vtable vt = {
            &destroy_op<F>, &move_op<F>, &copy_op<F>,
            &clone_op<F>, &deallocate_op, &invoke_op<F>
        };
        return &vt;
    }

    template <typename F>
    void assign(F&& f) {
        using T = std::decay_t<F>;
        static_assert(std::is_copy_constructible_v<T>,
                      "mini_function 要求可调用对象可拷贝（同 std::function）");
        if constexpr (fits_sbo<T>) {
            ::new (static_cast<void*>(sbo_)) T(std::forward<F>(f));
            data_ = sbo_;
        } else {
            data_ = new T(std::forward<F>(f));
        }
        vt_ = get_vtable<T>();
    }

    void copy_from(const mini_function& o) {
        if (!o) return;
        if (o.data_ == o.sbo_) { o.vt_->copy_construct(o.sbo_, sbo_); data_ = sbo_; }
        else                   { data_ = o.vt_->clone(o.data_); }
        vt_ = o.vt_;
    }

    void move_from(mini_function& o) noexcept {
        if (!o) return;
        if (o.data_ == o.sbo_) { o.vt_->move_construct(o.sbo_, sbo_); data_ = sbo_; }
        else                   { data_ = o.data_; }   // 堆：只搬指针
        vt_ = o.vt_;
        o.reset();
    }

    void reset() noexcept {
        if (!vt_) return;
        if (data_ == sbo_) { vt_->destroy(sbo_); }
        else               { vt_->destroy(data_); vt_->deallocate(data_); }
        data_ = nullptr;
        vt_ = nullptr;
    }
};
