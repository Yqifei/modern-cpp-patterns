# 类型擦除（Type Erasure）

> 模块 01。对应实现：`mini_function.h`（函数指针表流派）、`simple_function.h`（虚继承流派）。

## 来源

类型擦除不在 GoF 23 之列，是 C++ 模板元编程时代的惯用法：

- 概念源头：Andrei Alexandrescu《Modern C++ Design》（2001）对基于策略/模板的类型操作总结；Boost.Any / Boost.Function 是标准库前身（2000 年代初）。
- 理念推广：Sean Parent 的著名演讲 "Inheritance Is The Base Class of Evil"（2013）——用 `std::function` / `std::any` 替代继承体系表达"可调用/可存储"。
- 标准库形态：C++11 `std::function`、C++17 `std::any`。Qt 侧对应 `QVariant` + `QMetaType`（元对象系统的一部分）。

一句话：**把"类型"从数据里抽走，让同一容器/接口承载任意类型，运行时再恢复**。

## 原理（内存层）

类型擦除有两种实现流派，本模块各写了一个：

### 虚继承流派（`simple_function.h`）

```
ICallable（抽象接口）←继承— ICallableImpl<T> { T callable }
SimpleFunction 持有 unique_ptr<ICallable>
```

- 每种类型一个派生包装对象，一律堆分配（`make_unique`）。
- 调用路径：`vptr → 虚表 → 函数`，两层间接。
- 拷贝：`clone()` 虚函数深拷贝堆对象（截图常见版本用 `unique_ptr` 禁拷贝，本模块补齐为可拷贝）。

### 函数指针表流派（`mini_function.h`，真实 std::function 架构）

```
vtable { destroy / move_construct / copy_construct / clone / deallocate / invoke }
void* data_ + SBO 缓冲（16 字节）
```

- 每种 T 实例化一组静态模板函数，函数地址装入同一张 vtable——**类型信息从数据搬进代码段**，对外只剩 `void*` + 函数指针。
- 小 callable（`sizeof ≤ 16 && 对齐满足 && nothrow 移动`）放对象内缓冲，零堆分配；太大才堆。
- 调用：一次函数指针间接跳转（比虚调用少一层）。
- 可拷贝：SBO 走 `copy_construct`，堆走 `clone`，两路径分离。

### 类型恢复

`std::any`：存 `type_info` 指针，`any_cast<T>` 先 `type() == typeid(T)` 再 `static_cast`。
`QVariant`：`QMetaType` 注册表按 typeId 索引，`value<T>()` / `canConvert<T>()`，带内置转换规则（int↔QString）。

## 使用场景

- **回调注册表 / 观察者**：任意 callable 共存一个容器（`NotificationCenter`），无需继承 Observer 接口。
- **策略注入**：策略即 `std::function`，lambda 即策略，替代 Strategy 类族。
- **事件总线 / 插件接口**：`post(const std::any&)`，事件类型开放。
- **异构值存储**：表格单元格、配置系统、参数传递（`std::any` / `QVariant`）。
- **Qt 特有**：信号槽参数、QML 边界、`QAbstractItemModel::data()`——必须 QVariant（元对象系统要求）。

## 最佳实践

1. **优先标准库**：`std::function` / `std::any` 已是生产级（SBO、异常安全、noexcept 移动）。手写版本只用于理解原理或 ABI/裁剪需求。
2. **能编译期就别擦除**：类型集合封闭 → `std::variant`（编译期安全、零堆、无 typeid 开销）；开放且必须运行时知道才用 any。
3. **小对象优化（SBO）是性能关键**：回调热点场景（逐帧、逐事件）小 lambda 应留在缓冲内，避免每次构造 malloc。
4. **拷贝语义必须完整**：可拷贝是回调容器的硬需求（观察者列表要能复制）；实现时注意拷贝/移动/析构三路径都要覆盖。
5. **Qt 侧用 QVariant，C++ 侧用 std::any**：需要转换/信号槽/序列化集成选 QVariant；纯存取选 std::any。
6. **自定义类型进 Qt**：`Q_DECLARE_METATYPE`（编译期资格）是基础，跨线程 queued connection 必须 `qRegisterMetaType`（运行期注册表）。

## 面试要点

- 类型擦除两流派差异：虚继承（一律堆、两层间接、需 clone 拷贝）vs 函数指针表（SBO 零分配、一次间接、天然可拷贝）。
- 真实 `std::function` 没有虚函数——函数指针表流派（libstdc++/MSVC 同款）。
- `std::any` 恢复 = `typeid` 比较 + `static_cast`；错误从编译期推迟到运行期（`any_cast` 判空）。
- `QVariant` 与 `std::any` 的本质区别：any 只存取，QVariant 有转换 + 元对象系统集成。
- 关联 GoF：类型擦除可视为"代理/桥接"的模板化变体；EventBus 用它承载异构事件。
