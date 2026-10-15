# PImpl / d-pointer（编译隔离 / ABI 稳定 / 隐藏实现）

> 模块 04。对应实现：`media_player.h`（接口 + unique_ptr<Impl>）、`media_player.cpp`（Impl 完整定义）。

## 来源

- **PImpl**（Pointer to Implementation）：C++ 社区经典惯用法，本质是"前向声明 + 指针成员"，将类的实现细节从头文件剥离到 .cpp。
- **d-pointer**：Qt 对 PImpl 的框架内封装（`Q_DECLARE_PRIVATE` / `Q_DECLARE_PUBLIC`，`QObject::d_ptr`）——所有 Qt 类（QWidget、QFile 等）都用它。
- 关联 GoF：结构上类似"桥接"（接口与实现分离），但动机是编译/ABI 治理而非运行时替换。

## 原理（内存层）

### 头文件只"知道有 Impl"，不"知道 Impl 是什么"

```
media_player.h（对外暴露的契约）：
    struct Impl;                    // 前向声明：告诉编译器"存在一个 Impl"
    std::unique_ptr<Impl> d_;       // d-pointer：8 字节指针

media_player.cpp（唯一知道完整类型的地方）：
    struct MediaPlayer::Impl {      // 完整定义：成员随便加，外部看不见
        QString path; bool playing; double volume; ...
    };
```

- `sizeof(MediaPlayer) = 8`：对象里只有一个指针，**大小不随实现成员变化**；
- 实现类完整定义只在 .cpp —— 外部代码（include 头文件的人）**无法访问、甚至无法看到** Impl 的成员；
- 所有方法实现转发给 `d_->...`，一次指针间接。

### 三个收益（对应三种痛点）

| 收益 | 机制 | 解决什么 |
|---|---|---|
| 编译隔离 | 改 Impl（.cpp）→ 头文件不变 → include 头文件的依赖方**不重编译** | 大项目里改一个类，全工程重编 |
| ABI 稳定 | 对象大小固定（一个指针），布局不变 | 二进制兼容：库升级不用重编客户端 |
| 隐藏实现 | Impl 成员对外不可见 | 内部状态/第三方依赖不泄露到头文件 |

### 代价（面试必问）

- 一次指针间接（`d_->x` 的解引用）+ 每次构造一次堆分配（`make_unique<Impl>`）；
- 头文件里 `unique_ptr` 的**析构/拷贝必须在 .cpp 定义**：`~MediaPlayer() = default` 写在 .cpp——因为销毁 `Impl` 需要完整类型，头文件里只有前向声明，编译器在那里无法生成析构代码；
- 拷贝需要深拷贝 `Impl`（完整类型在 .cpp，所以拷贝构造也在 .cpp 手写）。

## 使用场景

- **大而稳定的对外接口**：库/SDK 的公开类（改内部实现不想破坏用户编译）；
- **第三方依赖隔离**：头文件不想 include 第三方头（如私有解码器句柄），依赖只出现在 .cpp；
- **Qt 生态**：QObject 派生类天然用 d-pointer；自定义 QObject 组件可仿照 Q_DECLARE_PRIVATE 结构；
- **编译耗时敏感的大项目**：头文件依赖树越小，增量编译越快（Filmora 这类大软件尤其在意）。

## 最佳实践

1. **析构必须在 .cpp 定义**：`~MediaPlayer() = default;` 写进源文件——这是 PImpl 的经典编译坑（头文件里 `= default` 会报 incomplete type）。
2. **移动默认、拷贝手写**：`unique_ptr` 可移动（搬指针），移动构造/赋值可 `= default`；拷贝必须深拷贝 Impl（在 .cpp）。
3. **暴露的公共接口保持稳定**：PImpl 的意义在于"公共接口不动、实现随便改"——频繁改接口会抵消收益。
4. **多态对象慎用**：PImpl 适合"具体类 + 隐藏实现"；需要虚函数多态的类，PImpl 要小心（vptr 位置/拷贝语义复杂化）。
5. **性能热点注意**：每次调用多一次指针间接；逐帧/逐元素的热点方法可考虑非虚转发 + 内联的替代方案。

## 面试要点

- PImpl 三收益：编译隔离 / ABI 稳定 / 隐藏实现；一次间接 + 一次堆分配是代价。
- 为什么析构必须放 .cpp：unique_ptr 析构需要 Impl 完整类型，头文件只有前向声明。
- 为什么拷贝要在 .cpp：深拷贝 Impl 同样需要完整类型。
- Qt d-pointer 就是 PImpl：`Q_DECLARE_PRIVATE` / `QObject::d_ptr`，所有 Qt 类在用。
- 与桥接模式区别：PImpl 动机是编译/ABI 治理（隐藏实现细节），桥接动机是运行时抽象替换（解耦抽象与实现）。
- 与代理/门面区别：PImpl 不是给别人当替身，是给自己藏实现。
- 关联：PImpl + RAII（unique_ptr 管理 Impl 生命周期）天然组合，正是规则之零的实例。
