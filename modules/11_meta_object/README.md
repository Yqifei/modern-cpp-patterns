# 模块 11：元对象系统（Meta-Object System）

> 对应实现（接口与实现分离）：`player_profile.h/.cpp`（Q_OBJECT 反射载体），`main.cpp`（五场景演示：MOC 产物 / 元对象表 / 属性系统 / 信号槽 / 反射调用）。

## 来源

Qt 特有机制（非 GoF 23）。1996 Trolltech 引入 MOC（Meta-Object Compiler），核心是**静态代码生成 + 运行时查表**：编译期用 `Q_OBJECT` 宏标记类，MOC 扫描头文件生成一份 `QMetaObject` 静态元数据（类名、继承链、属性表、信号槽表、可调用方法表），运行时通过 `metaObject()` 查这张表——这就是 Qt 的反射。信号槽、Q_PROPERTY、Q_INVOKABLE、QML 绑定、QObject::property() 全部建立在它之上。

## 用法

```cpp
class PlayerProfile : public QObject {
    Q_OBJECT                                     // MOC 生成元对象表
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)  // 属性入表
public:
    Q_INVOKABLE void levelUp(int delta = 1);      // 方法入表（可字符串调用）
signals:
    void leveledUp(int newLevel);                 // 信号入表
};

// 运行时反射
profile.metaObject()->className();                          // 类型名
meta->property(i).name();  meta->method(i).methodSignature();  // 遍历属性/方法表
profile.setProperty("name", "Jungle");                      // 动态写属性（走 WRITE）
QMetaObject::invokeMethod(&profile, "levelUp", Q_ARG(int, 2));  // 字符串寻址调用
QObject::connect(&profile, &PlayerProfile::leveledUp, ...);     // 信号槽
```

## 使用场景

- **反射与序列化**：遍历属性表把对象 dump 成配置/JSON（不用手写每个字段）；
- **插件系统**：`metaObject()->className()` 识别插件类型、查接口方法（模块 13 的底座）；
- **事件总线 / 命令分发**：`invokeMethod` 按字符串动态调用（模块 08b 的跨线程投递）；
- **QML 绑定**：`Q_PROPERTY + NOTIFY` 是 QML 属性绑定的数据来源（模块 10 MVVM）；
- **编辑器 / 属性面板**：遍历元对象表自动生成属性编辑控件；
- **调试与诊断**：运行时打类型名、方法签名，不用 RTTI。

## 原理（内存层）

### MOC 生成什么

```
Q_OBJECT 类源码
   │ MOC 扫描
   ▼
moc_player_profile.cpp（自动生成，链接进程序）
   - 静态 QMetaObject 实例：类名、superClass、字符串表、方法索引表、属性表
   - metaObject() / qt_metacall() / qt_static_metacall() 实现
```

每个 Q_OBJECT 类**编译期静态生成**一张元对象表，放在程序只读数据段（.rodata 附近）——不是运行时反射框架，是"编译期写死、运行时查表"。

### QMetaObject 结构（内存视角）

```
QMetaObject（静态实例）
 ├── className        "PlayerProfile"
 ├── superClass        → QObject 的 QMetaObject（继承链）
 ├── 字符串表          属性名/方法签名的字符串池
 ├── 属性表            name/level：READ/WRITE/NOTIFY 索引
 ├── 方法表            leveledUp(信号)、levelUp(槽/方法)、summary(方法)
 └── 索引规则          信号、槽、方法按顺序编号；propertyOffset/methodOffset
                       = 继承来的数量，从 offset 起才是自己声明的
```

- `metaObject()->className()` 查的就是这张静态表；
- `propertyOffset/methodOffset`：**继承链上的每个类都往表里追加自己的属性/方法**，offset 标记"从哪条起是我的"——遍历时 `[offset, count)` 就是本类声明的（场景 2 演示）；
- 对象本身不存元数据——`metaObject()` 是虚函数，返回类的静态表地址，**一个类只存一份表，所有实例共享**（内存零浪费）。

### 信号槽派发路径

```
emit leveledUp(3)
   → 根据编译期算好的信号索引（methodIndex）
   → qt_static_metacall / qt_metacall：查连接表，按索引派发给槽
   → DirectConnection：同线程直接调槽函数
   → QueuedConnection：打包事件投递到接收线程事件循环（跨线程）
```

- 连接在 `connect()` 时建立（信号索引 ↔ 槽函数映射），emit 时按索引查表派发——**比纯虚函数多一次索引查表，这是"信号槽比回调略慢"的来源**；
- 类型安全：信号参数经元类型注册（`QMetaType`）校验，参数不匹配在 connect 时拒绝。

### Q_OBJECT 类为什么不能拷贝

`QObject` 有身份语义（对象名、parent、信号连接、元对象），内部持 `QObjectData* d_ptr`——拷贝一个对象等于复制一份"身份"，信号连接和父子关系无法复制。所以 `QObject` 及其子类拷贝构造/赋值被禁用（`Q_DISABLE_COPY`）。工程上：QObject 子类要么用指针/引用传递，要么包进 `std::unique_ptr`。

## 现代 C++ / Qt 最佳实践

- **Q_PROPERTY 规范**：getter 用 `const`，WRITE 里值没变就不要 emit NOTIFY（防无谓刷新）；NOTIFY 信号参数与属性类型一致；
- **需要反射才用 Q_OBJECT**：普通工具类不需要元对象就别加（省 MOC 开销）；
- **invokeMethod 用于动态/跨线程调用**：同线程同类型直接用函数指针 connect，别用字符串（快且编译期查错）；
- **遍历元对象表注意 offset**：从 `propertyOffset()/methodOffset()` 开始，否则打印一堆 QObject 继承来的成员；
- **动态属性（setProperty 任意名）**：只存运行时表，Q_PROPERTY 查不到——用于给对象临时挂标记（如权限标签），别当正式字段用。

## 面试要点

- **Q1：Q_OBJECT 宏做了什么？** 展开后声明 `metaObject()/qt_metacall()/tr()` 等，MOC 扫描它生成静态 QMetaObject 表——类名、继承链、属性表、信号槽表。
- **Q2：信号槽机制原理？** connect 建立"信号索引 ↔ 槽"映射；emit 走 qt_metacall 按索引派发；Direct 同线程直调，Queued 打包投事件循环（跨线程）。
- **Q3：为什么信号槽比回调慢一点？** 多一层索引查表/类型检查；但换来类型安全、自动断开（context 对象析构）、线程安全投递。
- **Q4：为什么 Q_OBJECT 类不能拷贝？** QObject 是身份语义（d_ptr + 信号连接 + 父子关系），拷贝无法复制身份，故禁用拷贝。
- **Q5：Q_PROPERTY 和普通成员变量的区别？** Q_PROPERTY 注册进元对象表：可被 property()/setProperty() 字符串访问、可序列化、可被 QML 绑定、NOTIFY 信号通知。
- **Q6：invokeMethod 什么时候用？** 跨线程投递（QueuedConnection）、按名字动态调用（插件/脚本）、QML 调 C++ 命令。
- **Q7：MOC 是代码生成还是运行时反射？** 元对象表是**编译期静态生成**（MOC 产物），运行时是查表——不是解释型反射框架，性能开销很小。
- **Q8：信号槽 vs 回调函数？** 回调：直接函数指针，快但类型不安全、断连要手工管；信号槽：索引派发 + 类型检查 + 自动断连 + 跨线程，慢一点点但工程上更安全。

## 演示输出说明

```
1) className / superClass         PlayerProfile → QObject（继承链）
2) 元对象表                       属性表 2 个（name/level，含 NOTIFY）；方法表含信号/槽/方法签名
3) 属性系统                       setProperty 走 WRITE；property 读回；动态属性 vip 无需声明
4) 信号槽                        levelUp → [槽] leveledUp；setName → [NOTIFY] nameChanged
5) 反射调用                       invokeMethod("levelUp",2) 生效；invokeMethod("summary") 带回返回值
```


### moc_文件简单说明：
![carbon.png](carbon.png)