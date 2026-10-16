# 模块 08：EventBus（集中式发布-订阅 / 模块解耦）

> 对应实现：`event_bus.h`（`EventBus`：type_index 路由 + 类型擦除回调 + 快照发布 + 强/弱订阅 + token 退订）。

## 来源

- **EventBus（事件总线）**：GoF 之外的补充模式（行为型）。观察者模式的一对多直连升级为**任意对任意**——发布者不认识订阅者，只发布"事件类型"；订阅者不认识发布者，只订阅"事件类型"。中间的"总线"负责路由。
- Qt 里的对应形态：信号槽是 **per-object 直连**的分布式观察者（编译期类型安全、自动断开）；`QEvent`/`QApplication::postEvent` 是事件队列；跨模块全局解耦时，EventBus 是信号槽的补充方案。
- 关联 GoF：观察者（直连版）、中介者（集中协调的简化版）。

## 用法

```cpp
EventBus bus;

// 订阅：回调签名就是事件类型，类型安全
auto tok = bus.subscribe<PlaybackStarted>([](const PlaybackStarted& e) {
    qDebug() << "播放:" << e.file;
});

// 发布：发布者不需要任何订阅者的引用
bus.publish(PlaybackStarted{"demo.mp4"});

bus.unsubscribe(tok);          // 按 token 退订

// 弱订阅：对象销毁后自动跳过（防悬空回调）
bus.subscribeWeak<PlaybackStarted>(moduleGuard, handler);
```

## 使用场景

- **跨模块解耦**：播放器、UI、日志、统计各自独立，通过事件通信——新增订阅者不改发布者任何代码（开闭原则）；
- **广播通知**：一个状态变化（播放开始/错误/进度）多个模块都要响应；
- **插件系统**：插件订阅感兴趣的事件，宿主只 publish，双方零依赖；
- **Qt 视角**：模块间全局解耦（信号槽要持有对方引用或靠 QObject 树），EventBus 让互不相识的模块通信；QObject 信号槽负责"已知对象间的直连"，EventBus 负责"未知对象间的广播"。

## 原理（内存层）

### 内部结构

```
EventBus
  ├─ table_：unordered_map<type_index, vector<Entry>>   事件类型 → 订阅列表
  │            Entry = { token, std::function<void(const void*)> }
  ├─ mtx_：订阅表锁
  └─ nextToken_：全局递增 id（token 唯一）
```

- **type_index 路由**：`typeid(E)` 在编译期求值，得到 `type_index` 作为 map 键——事件类型即路由地址，O(1) 查找；
- **回调类型擦除**：订阅回调 `void(const E&)` 被包装成统一的 `void(const void*)` 存进 `std::function`（模块 01 的类型擦除）——容器不需要知道 E 是什么，发布时 `static_cast<const E*>` 还原；
- **快照发布**：`publish` 先在锁内**拷贝**订阅列表快照，解锁后再逐个执行回调——两个工程收益：回调里再 `publish`（不重复加锁，不死锁）；回调里 `unsubscribe`（快照已拷贝，迭代器不失效）；
- **弱订阅**：回调里先 `guard.lock()`——`weak_ptr` 提升成功说明订阅者对象还活着，短暂托住后再调用；提升失败（已销毁）直接跳过。**悬空回调的运行时防线**。

### 一次 publish 的开销

```
publish<E>：锁 + type_index 查找 + 快照拷贝（vector<Entry>）+ N 次回调
其中快照拷贝是主要成本（N 个 std::function 的拷贝）
→ 订阅者多、事件高频时，快照拷贝成为热点（工程上可优化为"发布期禁止退订"或引用计数方案）
```

## 现代 C++ 最佳实践

- **subscribe<E> 模板让回调签名类型安全**：编译期校验"回调参数 == 事件类型"，不必手动 cast；
- **回调里再 publish 必须依赖快照机制**：不在锁内执行用户回调是铁律（否则死锁）；
- **弱订阅用于"观察者生命周期短于发布者"**的场景；订阅者生命周期由总线持有（shared_ptr）时可强订阅；
- **token 退订**：退订 O(所有订阅)（遍历各类型表）；订阅多可换 per-type 自增 id 或句柄；
- **事件类型是普通 struct**：只携带数据，不含行为（避免回调里执行重活，重活应投递到事件循环）；
- **别滥用**：全局总线会把所有模块揉进同一个依赖——调试靠事件日志；高频事件（如逐帧进度）别走总线，直接调用。

## 面试要点

- **Q1：EventBus 和观察者模式区别？** 观察者是一对多直连：发布者持有订阅者列表，双方有引用关系；EventBus 是任意对任意：发布者只发布事件类型，不认识订阅者，中间由总线路由——解耦更彻底，代价是多一层间接。
- **Q2：EventBus 和 Qt 信号槽区别？** 信号槽是 per-object 直连（编译期类型安全、对象销毁自动断开、支持 queued 跨线程）；EventBus 是全局广播（运行期 type_index 路由、零引用解耦、需要自己管理生命周期与线程）。已知对象通信用信号槽，未知对象广播用 EventBus。
- **Q3：为什么 publish 先拷贝快照？** 锁内执行用户回调会死锁（回调里再 publish 会重入锁）；快照拷贝后解锁执行，回调里 publish/退订都安全——这是并发 EventBus 的关键设计。
- **Q4：怎么防悬空回调？** 弱订阅：回调捕获 weak_ptr（订阅者对象），执行时 lock() 提升，失败跳过。释放时机由对象自身生命周期决定，而非总线。
- **Q5：事件类型怎么路由？** typeid(E) 编译期得到 type_index 作 map 键，O(1) 定位订阅列表；回调统一擦除为 void(const void*)，发布时还原。
- **Q6：EventBus 的缺点？** 全局耦合（所有模块依赖同一个总线）、调试困难（事件流分散）、高频事件开销（快照拷贝）、滥用会变成"上帝对象"。工程上限定事件集合 + 事件日志 + 低频广播。
