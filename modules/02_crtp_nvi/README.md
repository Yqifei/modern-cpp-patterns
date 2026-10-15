# CRTP + NVI（静态多态 / 非虚接口）

> 模块 02。对应实现：`crtp_processor.h`（CRTP 帧处理器族 + 虚函数对照）、`nvi_exporter.h`（NVI 导出任务骨架）。

## 来源

两个都是 C++ 惯用法（idiom），不在 GoF 23 之列：

- **CRTP**：James Coplien 1995 年在 C++ Report 提出 "Curiously Recurring Template Pattern"，描述"类模板以派生类为模板参数"这一奇异形态；现代最广为人知的实例是 `std::enable_shared_from_this<T>`（C++11）。
- **NVI**：Scott Meyers《Effective C++》（条款 35）与 Herb Sutter 的 "Virtuality"（2001）共同推广——public 接口非虚，虚函数收进 private/protected。"接口即契约"思想的强化版模板方法。

## 原理（内存层）

### CRTP：编译期分派，无 vptr

```
GammaProcessor : public FrameProcessor<GammaProcessor>
                  ↑ 基类模板参数 = 派生类自己

FrameProcessor<GammaProcessor>::process():
    static_cast<GammaProcessor*>(this)->processImpl(frame)
```

- 基类是模板，实例化时 `Derived` 已是具体类型 → 基类**反向调用**派生类方法，编译期钉死，不查表。
- 对象无 vptr（`sizeof(GammaProcessor) = 256`：256B 查找表纯数据），虚函数版空实现也要 8 字节 vptr。
- 调用可内联，零运行时开销；代价：类型必须编译期确定，不能进异构容器（`vector<Base*>`）。
- 空基类优化（EBO）：无数据成员的非虚基类不占空间，纯 mixin 场景对象零膨胀。

### NVI：外壳非虚，虚实现收进 protected

```
public  bool run()        ← 非虚外壳（前置校验 + 后置清理，焊死）
protected virtual doRun() ← 子类唯一覆写点
private   bool prepare()  ← 公共逻辑，子类碰不到
```

- 重写判定只看**函数签名**，不看访问级别——private/protected 虚函数同样能被子类 override；访问控制只管"调用权"。
- 效果：公共前置/后置逻辑**物理上不可绕过**（失败也执行清理，模块输出已验证）。
- 与模板方法区别：模板方法骨架本身是虚的（子类可覆写整个骨架），NVI 骨架非虚（只换实现）。

## 使用场景

- **CRTP 主场**：逐帧/逐元素热点调用（DSP 滤波器、像素格式处理、SIMD 后端）、编译期类型族共享外壳逻辑、`enable_shared_from_this` 这类"借基类补充功能"的 mixin。
- **NVI 主场**：框架对外接口需要"强制公共逻辑"——日志埋点、锁/RAII、校验、审计、前后置统计；子类实现细节需要收权。
- **组合用法**：基类外壳按 NVI 设计（公共流程焊死），分派用 CRTP（编译期零开销），即本模块 `FrameProcessor::process` 的形态。

## 最佳实践

1. **热点 + 类型确定 → CRTP；开放 + 运行时替换 → 虚函数**。不要为"显得高级"用 CRTP，异构容器场景它是负资产。
2. **CRTP 基类析构设为 protected**：无虚析构，防止通过基类指针 delete 派生对象（否则 UB/泄漏）。
3. **用 friend 而非 public 暴露实现**：基类要调 `processImpl`，保持派生实现私有，friend 授权即可。
4. **NVI 外壳保持小且稳定**：前置/后置做"必要的公共逻辑"，不要堆砌无关代码；子类自由度被压缩，外壳膨胀会逼子类绕道。
5. **private virtual 是原教旨，protected virtual 是常态**：子类需要复用实现时用 protected（Qt 大量如此）；要绝对收权用 private。
6. **避免 CRTP 滥用导致编译期爆炸**：模板实例化数量随派生类线性增长，合理拆分，别把所有逻辑都塞进一个模板基类。

## 面试要点

- CRTP vs 虚函数：编译期分派（无 vptr、可内联、ABI 稳）vs 运行期查表（对象 +8B、两层间接、类型异构）。
- `static_cast<Derived*>(this)` 为什么安全：实例化时 Derived 就是真实类型，方向正确。
- NVI vs 模板方法：骨架非虚（契约焊死）vs 骨架虚（步骤可换）。
- private 虚函数能被重写：重写看签名，访问控制只管调用权——这正是 NVI 成立的前提。
- 构造/析构期间虚调用不产生多态（vptr 逐层切换），NVI 外壳调用虚实现时不存在此问题（对象已构造完成）。
- 关联 GoF：CRTP 可视为"模板方法"的静态版；NVI 是模板方法的契约强化版。
