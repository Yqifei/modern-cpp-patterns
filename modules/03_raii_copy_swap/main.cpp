// RAII（资源即生命周期）与 Copy-and-Swap（复制交换）演示
//
// 场景：
//   A. RAII —— 文件句柄 / 作用域锁：正常返回与异常路径都释放资源
//   B. Copy-and-Swap —— 帧缓冲：拷贝 / 移动 / 自赋值 / 拷贝隔离

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include <mutex>
#include <stdexcept>

#include "win_console.h"

#include "copy_swap_buffer.h"
#include "raii_scope.h"

static void scene_raii();
static void scene_copy_swap();

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    scene_raii();
    scene_copy_swap();
    return 0;
}

static void writeAndReturn(bool fail) {
    FileHandle f("out.bin");
    f.write("data", 4);
    if (fail) return;   // 提前返回：析构照样执行，文件照样关闭
    qDebug() << "  正常路径完成";
}

static void lockedSection(bool throwFlag) {
    std::mutex m;
    try {
        ScopedLock lock(m);
        qDebug() << "  临界区开始（已加锁）";
        if (throwFlag) throw std::runtime_error("临界区内异常");
        qDebug() << "  临界区结束";
    } catch (const std::exception& e) {
        qDebug() << "  捕获:" << e.what();
    }   // 锁在作用域出口释放——无论是否抛异常
    qDebug() << "  离开作用域，锁已释放";
}

static void scene_raii() {
    qDebug() << "\n--- RAII：资源即生命周期 ---";
    qDebug() << "  [路径 1] 正常返回";
    writeAndReturn(false);
    qDebug() << "  [路径 2] 提前返回（模拟异常路径）";
    writeAndReturn(true);

    qDebug() << "  [锁] 异常安全释放";
    lockedSection(false);
    lockedSection(true);
}

static void scene_copy_swap() {
    qDebug() << "\n--- Copy-and-Swap：异常安全的赋值 ---";
    FrameBuffer a(4096);
    qDebug() << "  a.size =" << a.size();

    FrameBuffer b(a);            // 拷贝构造
    qDebug() << "  b = 拷贝 a，size =" << b.size();

    FrameBuffer c;
    c = a;                       // copy-and-swap 拷贝赋值
    qDebug() << "  c = a，size =" << c.size();

    FrameBuffer d(std::move(c)); // 移动构造
    qDebug() << "  d = 移动 c，size =" << d.size() << "，c.size =" << c.size();

    d = d;                       // 自赋值：安全（先拷贝再交换）
    qDebug() << "  自赋值 d = d，size 不变 =" << d.size();

    qDebug() << "  sizeof(FrameBuffer) =" << sizeof(FrameBuffer)
             << "字节（两个指针宽：data_ + size_）";
}
