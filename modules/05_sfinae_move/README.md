# 模块 05：SFINAE + 移动语义（编译期选路 / 零拷贝搬移）

> 对应实现：`sfinae_helpers.h`（enable_if 重载、void_t 探测、requires 对照、引用折叠）、`move_aware_buffer.h`（独占堆缓冲的拷贝/移动/swap）。

## 来源

- **SFINAE**（Substitution Failure Is Not An Error）：C++98 时代就存在的模板机制。模板替换（实例化）时某个候选的替换结果非法，编译器不报错，只剔除该候选，其余重载继续决议。经典应用：`std::enable_if`、`void_t` 检测惯用法、`std::is_*` 类型特征。
- **移动语义**：C++11 引入右值引用后的核心能力。`std::move` 无条件转右值，`std::forward` 按原值类别转发；`noexcept` 移动构造让容器可以放心用"搬指针"代替"深拷贝"。
- 现代替代：C++20 `requires` / concepts 让"按能力选路"可读性更好，但 SFINAE 仍被标准库和旧代码大量使用，面试考察的是机制理解。

## 用法

```cpp
// SFINAE 选路：整数走 describe(int)，浮点走 describe(double)，编译期决定
qDebug() << sfinae::stringify(42);               // "42"
qDebug() << sfinae::stringify(Version{2, 7});    // "2.7"（有 toString 走 toString）

// void_t 探测：类型是否有成员
static_assert(sfinae::has_to_string_v<Version>);
static_assert(!sfinae::has_to_string_v<int>);

// 移动：零拷贝搬移
MoveBuffer a(64 * 1024);
MoveBuffer b = std::move(a);   // 只搬指针，a 置空
```

## 使用场景

- **序列化 / 日志格式化**：一个 `stringify` 同时支持"有 toString() 的类型"和"原生数值类型"，新增类型只要实现 toString() 就自动接入（开闭原则的编译期版本）；
- **重载选择**：同一函数名按类型特征（整数/浮点、可拷贝/可移动、有无某成员）选不同实现，避免写多个函数名；
- **资源转移**：独占资源（堆缓冲、句柄、unique_ptr）的传递一律用移动——拷贝意味着复制资源，移动意味着转让所有权；
- **容器扩容**：`std::vector` 扩容时对元素做搬移，`noexcept` 移动保证搬移路径被选中且 O(1)；
- **Qt 视角**：QString/QByteArray 是隐式共享（COW），拷贝也便宜，但移动是常量时间且不触发引用计数；Qt6 全面补齐移动构造/移动赋值，自定义 QObject 数据类建议同样提供。

## 原理（内存层）

### SFINAE 的机制

```
template <typename T>
std::enable_if_t<has_to_string_v<T>, std::string> stringify(const T& v);

调用 stringify(42)：
  ① 替换 T = int，求值 has_to_string_v<int>
  ② decltype(declval<const int&>().toString()) → int 没有 toString() → 表达式非法
  ③ 替换失败 → 该候选被剔除（不报错）
  ④ 第二个重载（!has_to_string_v<T>）替换成功 → 选中

调用 stringify(Version{...})：方向相反，第一个命中。
```

替换失败发生在"模板实参替换"阶段，早于重载决议和函数体实例化——所以非法只淘汰候选，不成为硬错误。`std::void_t<T>` 的价值：把任意"表达式是否合法"折叠成 `void` 这一种形态，便于特化匹配。

### 移动 vs 拷贝的堆行为

```
拷贝 MoveBuffer copy = big;
  big.data(0x7f..a0) ──────┬────── 新分配 0x7f..b0（memcpy 64KB）
  两份数据都在，源不动       │
                            ↓
移动 MoveBuffer moved = std::move(copy);
  moved.data = copy.data（同一个地址，只搬指针）
  copy.size = 0，copy.data = nullptr（源置空，合法但未指定）
  零分配，O(1)
```

### noexcept 为什么是移动的黄金法则

`std::vector` 扩容逻辑（move_if_noexcept 语义）：元素具备 `noexcept` 移动构造时用移动（搬指针，O(1)，且中途失败不破坏原容器）；否则回退拷贝（保证强异常安全：要么成功要么原样）。所以：

- 移动构造标 `noexcept` → 扩容 = 批量搬指针；
- 不标 → 明明可以搬却被迫深拷贝，性能退化一个量级。

### 引用折叠 / 完美转发

```
template <typename T> void relay(T&& v);
relay(x)  → T = int&，  T&& 折叠为 int&  （左值保持左值）
relay(1)  → T = int，   T&& 就是 int&&    （右值保持右值）
std::forward<T>(v) 按 T 恢复原始值类别转发；std::move 无条件转右值。
```

## 现代 C++ 最佳实践

- 移动构造/移动赋值**必须 noexcept**（凡能承诺的），这是容器优化的前提；
- 移动后源对象置空，保证"合法但未指定"且可析构、可重新赋值；
- 资源类优先"默认移动 + 删拷贝"（unique_ptr 模式），需要拷贝才手写深拷贝；
- 拷贝赋值优先 copy-and-swap（异常安全、自赋值安全），移动赋值记得释放旧资源；
- C++20 新代码优先 `requires` 表达约束，SFINAE 保留给需要兼容老编译器的库代码；
- 检测惯用法（has_xxx）是 traits 的骨架，`std::ranges` / 概念库内部大量依赖它。

## 面试要点

- **Q1：SFINAE 是什么？** 模板替换失败不是错误——替换阶段非法的候选被剔除，重载决议继续。是"编译期按能力选路"的机制基础。
- **Q2：enable_if 和 requires 什么关系？** 表达同一类约束的两种语法：enable_if 是替换期剔除候选，requires 是约束检查后剔除；前者可读性差但兼容 C++11，后者是 C++20 首选。
- **Q3：为什么移动构造要标 noexcept？** vector 扩容用 `move_if_noexcept`：noexcept 移动 → 搬指针 O(1)；否则回退拷贝以保证强异常安全。不标 noexcept 让容器不敢用移动。
- **Q4：std::move 和 std::forward 区别？** move 无条件转右值；forward 按模板参数 T 的推导结果恢复原始值类别（左值转发左值、右值转发右值），用于完美转发。
- **Q5：移动后源对象处于什么状态？** 合法但未指定（valid but unspecified）：可析构、可重新赋值，内容通常置空——标准做法是置空指针和大小。
- **Q6：拷贝和移动各分配多少次堆内存？** 拷贝 = 1 次新分配 + memcpy；移动 = 0 次（只搬指针 + 置空源）。
