#pragma once

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

// 管道-过滤器（Pipe-Filter）：把数据处理拆成独立步骤，数据流经管道。
//
// 两种形态：
//   ① IFilter 接口版（经典架构模式）：每个过滤器一个类，单一职责，可替换可测试
//   ② Pipeline<T> 函数式版（现代 C++）：std::function 串联，类型安全零继承
//
// 数据流：T → f1 → f2 → ... → fn → T'
// 过滤器不知道上下游是谁，只处理一帧数据。

// ---- 形态①：过滤器接口（带帧计数，供证据输出） ----
template <typename T>
class IFilter {
public:
    virtual ~IFilter() = default;

    virtual T process(T data) = 0;
    virtual const char* name() const = 0;

    std::size_t frames() const { return m_frames; }

protected:
    std::size_t m_frames = 0;   // 已处理帧数
};

// 管道：按序串联过滤器（非拥有，过滤器由调用方管理生命周期）
template <typename T>
class FilterChain {
public:
    FilterChain& add(IFilter<T>* filter) {
        filters_.push_back(filter);
        return *this;
    }

    T process(T input) const {
        for (auto* f : filters_) {
            input = f->process(std::move(input));
        }
        return input;
    }

    std::size_t size() const { return filters_.size(); }

private:
    std::vector<IFilter<T>*> filters_;
};

// ---- 形态②：函数式管道，lambda 直接组合 ----
template <typename T>
class Pipeline {
public:
    using Filter = std::function<T(T)>;

    Pipeline& add(Filter filter) {
        filters_.push_back(std::move(filter));
        return *this;
    }

    T run(T input) const {
        for (const auto& f : filters_) {
            input = f(std::move(input));
        }
        return input;
    }

    std::size_t size() const { return filters_.size(); }

private:
    std::vector<Filter> filters_;
};
