#pragma once

#include <QByteArray>
#include <QDebug>
#include <QElapsedTimer>
#include <QString>

#include <cmath>

// CRTP（Curiously Recurring Template Pattern）：
// 基类模板以"派生类自己"为模板参数，通过 static_cast<Derived*>(this)
// 把调用在编译期分派到派生实现 —— 静态多态：无虚表、无 vptr、可内联。
//
// 适用：类型在编译期确定、调用是热点路径（如逐帧处理）。
// 不适用：类型需要进异构容器（vector<Base*>）做运行时多态的场合。
template <typename Derived>
class FrameProcessor {
public:
    // 外壳：公共流程（计时）+ 编译期分派到 processImpl
    int process(const QByteArray& frame) {
        QElapsedTimer timer;
        timer.start();
        const int result = static_cast<Derived*>(this)->processImpl(frame);
        qDebug().noquote() << QString("  [%1] %2 字节 → %3，耗时 %4 ms")
                .arg(kind(),
                     QString::number(frame.size()),
                     result == 0 ? QStringLiteral("OK") : QStringLiteral("FAIL"),
                     QString::number(timer.elapsed()));
        return result;
    }

    static QString kind() { return Derived::kindImpl(); }

protected:
    ~FrameProcessor() = default;   // 防止通过基类指针裸析构（无虚析构的代价）
};

// 伽马校正：构造时生成 256 项查找表，逐字节查表变换
class GammaProcessor : public FrameProcessor<GammaProcessor> {
public:
    explicit GammaProcessor(double gamma) {
        for (int i = 0; i < 256; ++i)
            table_[i] = static_cast<uchar>(255 * std::pow(i / 255.0, 1.0 / gamma));
    }

private:
    friend class FrameProcessor<GammaProcessor>;

    int processImpl(const QByteArray& frame) {
        int sum = 0;
        for (const char c : frame)
            sum += table_[static_cast<uchar>(c)];
        return frame.isEmpty() ? -1 : 0;
    }

    static QString kindImpl() { return QStringLiteral("Gamma"); }

    uchar table_[256];   // 256 字节数据，但对象里没有 vptr
};

// 降噪：亮度均值统计（简化演示）
class NoiseReduce : public FrameProcessor<NoiseReduce> {
private:
    friend class FrameProcessor<NoiseReduce>;

    int processImpl(const QByteArray& frame) {
        qint64 sum = 0;
        for (const char c : frame)
            sum += static_cast<uchar>(c);
        return frame.isEmpty() ? -1 : 0;
    }

    static QString kindImpl() { return QStringLiteral("NoiseReduce"); }
};

// 对照：虚函数多态（动态分派）。
// 对象含 vptr（sizeof 8）；调用走 vptr → 虚表 → 函数，两层间接，无法内联。
class VirtualProcessor {
public:
    virtual ~VirtualProcessor() = default;
    virtual int process(const QByteArray& frame) = 0;
    virtual QString kind() const = 0;
};
