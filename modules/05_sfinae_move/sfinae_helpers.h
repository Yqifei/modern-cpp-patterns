#pragma once

#include <string>
#include <type_traits>
#include <utility>

// 模块 05 · SFINAE：Substitution Failure Is Not An Error（替换失败不是错误）
//
// 模板实例化做参数替换时，若某个候选的替换结果非法（表达式不成立、
// 类型不存在），编译器不报错，只把该候选从重载集里剔除——其余候选
// 继续参与决议。这层机制让"编译期按类型能力选路"成为可能。

namespace sfinae {

// ① enable_if 重载选择：整数 / 浮点各走一路
template <typename T>
std::enable_if_t<std::is_integral_v<T>, std::string> describe(T v) {
    return "integral " + std::to_string(v);
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string> describe(T v) {
    return "floating " + std::to_string(v);
}

// ② void_t 探测：类型有没有 toString() 成员
//    decltype(declval<const T&>().toString()) 合法 → void_t 得 void → 命中特化 → true
//    非法 → 替换失败（SFINAE）→ 落回主模板 → false
template <typename, typename = void>
struct has_to_string : std::false_type {};

template <typename T>
struct has_to_string<T, std::void_t<decltype(std::declval<const T&>().toString())>>
    : std::true_type {};

template <typename T>
inline constexpr bool has_to_string_v = has_to_string<T>::value;

// ③ stringify：按能力选路（有 toString 走 toString，否则走 std::to_string）
template <typename T>
std::enable_if_t<has_to_string_v<T>, std::string> stringify(const T& v) {
    return v.toString();
}

template <typename T>
std::enable_if_t<!has_to_string_v<T>, std::string> stringify(const T& v) {
    return std::to_string(v);
}

// ④ C++20 requires 对照：同一件事的现代写法
template <typename T>
    requires std::is_integral_v<T>
std::string describe20(T v) {
    return "requires integral " + std::to_string(v);
}

// ⑤ 完美转发 / 引用折叠：T&& 传左值时 T 推导为 U&（折叠回 U&），
//    传右值时 T 推导为 U（保持 U&&）
template <typename T>
std::string classify(T&&) {
    return std::is_lvalue_reference_v<T> ? "lvalue (T = U&)" : "rvalue (T = U)";
}

} // namespace sfinae
