# RAII + Copy-and-Swap（资源管理 / 异常安全赋值）

> 模块 03。对应实现：`raii_scope.h`（文件句柄 / 作用域锁）、`copy_swap_buffer.h`（帧缓冲）。

## 来源

- **RAII**：Bjarne Stroustrup 在 1980 年代提出（C++ 语言自带的思想，非 GoF 模式）——资源生命周期绑定对象生命周期。标准库体现：`std::lock_guard`、`std::unique_ptr`、`std::fstream`。
- **Copy-and-Swap**：C++ 异常安全讨论的经典惯用法（Dave Abrahams 的异常安全三保证、Herb Sutter GotW 系列推广）——用"按值传参 + 不抛异常的 swap"实现拷贝赋值，天然获得强异常安全。

## 原理（内存层）

### RAII：栈展开时析构必然执行

```
作用域进入 → 构造 FileHandle（fopen 获取 FILE*）
函数体 → 正常 return 或抛异常
作用域出口 → 析构必然执行（fclose 释放）
```

- 栈展开（stack unwinding）是 C++ 的运行时保证：**只要对象在栈上，离开作用域（无论正常还是异常路径）析构必被调用**；
- 所以资源释放写在析构里 = 释放操作"不可能被跳过"；
- 对比手动管理：`close()` 写在 return 前——异常路径一炸，close 被跳过，资源泄漏。

### Copy-and-Swap：传值 + swap 的异常安全

```cpp
FrameBuffer& operator=(FrameBuffer o) noexcept {   // o 是实参的拷贝
    swap(o);                                        // 只交换指针/大小，不抛异常
    return *this;
}
```

- **拷贝动作发生在参数构造**（`o` 的拷贝构造）：这一步可能抛异常（new 失败）——但此时 `this` 尚未被修改 → **强异常安全**（赋值失败，原对象不变）；
- **swap 不抛异常**：只交换两个指针 + 一个 size，`noexcept` 成立；
- 交换后 `o` 持有旧资源，**o 的析构负责释放**（作用域出口）；
- **自赋值天然安全**：`c = c` 时先拷贝再交换，没有"先析构自己再拷贝"的悬空窗口；
- **传右值时参数走移动构造**：同一个函数同时服务拷贝赋值与移动赋值。

### 为什么移动构造可以"默认资源 + swap"

```cpp
FrameBuffer(FrameBuffer&& o) noexcept : data_(nullptr), size_(0) {
    swap(o);   // 把 o 的内容搬给自己，o 变为空（空对象安全）
}
```

构造一个合法的空对象，再与 `o` 交换——`o` 变成空对象，析构时 `delete[] nullptr` 安全。这是"通过 swap 统一搬移"的常见手法，避免在两个构造里重复写成员搬运。

## 使用场景

- **RAII**：一切需要"获取/释放配对"的资源——文件、锁、内存、数据库连接、GPU 句柄、线程 join。Qt 里 `QFile`、`QMutexLocker`、`QSaveFile` 都是 RAII 实例。
- **Copy-and-Swap**：带堆资源的类（缓冲、容器、句柄包装）需要"可拷贝 + 异常安全赋值"时——帧缓冲、纹理数据、音频样本缓冲这类媒体软件高频对象。
- **规则之三/五/零**：资源类必须成对实现析构/拷贝/移动（三或五）；能用 `unique_ptr`/`shared_ptr` 自动管理的就别手写（规则零）。

## 最佳实践

1. **优先规则零**：能靠智能指针/标准容器管理资源，就别手写析构拷贝——手写越多越容易错。
2. **析构绝不抛异常**：析构里 `fclose`/`delete`/`unlock` 都不抛；若有错误，记录而不是抛出（析构抛异常在栈展开期会触发 terminate）。
3. **swap 必须 noexcept**：copy-and-swap 依赖它；`noexcept swap` 也是 `std::swap` 优化的前提。
4. **拷贝赋值传值（copy-and-swap）或传引用 + 拷贝交换都行**：传值写法最简洁，代价是多一次临时对象（可被移动优化吸收）。
5. **移动构造不要抛**：容器扩容依赖 noexcept 移动（同模块 02 SimpleFunction 的 `= default` 移动）。
6. **禁用拷贝时用 delete 而不是声明私有**：`= delete` 让错误在编译期、调用点暴露。

## 面试要点

- RAII 为什么是 C++ 异常安全的基础：栈展开保证析构执行 → 资源释放不可跳过 → 不需要 finally。
- 智能指针是 RAII 的实现，不是 RAII 本身；`lock_guard`/`QFile`/`unique_ptr` 全是同一思想。
- Copy-and-Swap 的强异常安全：拷贝在参数构造完成（失败则 this 不动），swap 不抛。
- 自赋值安全为什么免费获得：先拷贝再交换，无自我析构窗口。
- 规则三/五/零：什么时候必须手写五件套，什么时候用零（智能指针/容器代）。
- 析构函数为什么不能抛异常：栈展开期二次异常 → `std::terminate`。
- 关联 GoF：RAII 常与"对象池/门面"组合（资源获取即初始化，释放自动）；Copy-and-Swap 与"代理"组合可做写时复制的实现基础（模块 12 隐式共享）。
