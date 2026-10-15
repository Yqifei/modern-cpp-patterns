// 模块 07：对象池 + QThreadPool
//
// 同一个思想的两个尺度：
//   对象池  —— 复用对象实例（避免反复构造/析构）
//   线程池  —— 复用执行线程（避免反复创建/销毁线程）
// 对象池针对"实例"，QThreadPool 针对"执行上下文"，都是复用。

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#include "win_console.h"

#include "object_pool.h"

// 构造代价高的对象（模拟数据库连接）：每次构造/析构计数
struct DbConnection {
    static inline std::atomic<int> constructed{0};
    static inline std::atomic<int> destroyed{0};
    int id = 0;
    explicit DbConnection(int n) : id(n) { ++constructed; }
    ~DbConnection() { ++destroyed; }
};

// QThreadPool 演示任务：打印执行线程 id
class PoolTask : public QRunnable {
public:
    void run() override {
        {
            std::lock_guard<std::mutex> lk(g_printMtx);
            qDebug().noquote() << "    任务在线程" << QThread::currentThreadId() << "执行";
        }
        QThread::msleep(30);   // 模拟工作
    }

    static inline std::mutex g_printMtx;
};

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    qDebug() << "\n--- 对象池：实例复用 ---";

    {
        std::vector<std::unique_ptr<DbConnection>> conns;
        for (int i = 0; i < 10; ++i) {
            conns.push_back(std::make_unique<DbConnection>(i));
            conns.clear();     // 模拟用完即弃
        }
        qDebug() << "  无池: 构造" << DbConnection::constructed.load()
                 << "次, 析构" << DbConnection::destroyed.load() << "次";
    }

    DbConnection::constructed = 0;
    DbConnection::destroyed = 0;

    {
        // 预建 3 个，10 次借还：串行场景下 3 个够用，不再新建
        ObjectPool<DbConnection> pool(
            [] { return std::make_unique<DbConnection>(0); }, 3);
        for (int i = 0; i < 10; ++i) {
            auto conn = pool.acquire();
            conn->id = i;              // 借出后按需使用
            pool.release(std::move(conn));   // 用完归还
        }
        qDebug() << "  有池: 构造" << DbConnection::constructed.load()
                 << "次（预建 3，10 次借用全部复用）";
    }

    qDebug() << "\n--- QThreadPool：线程复用 ---";

    QThreadPool pool;
    pool.setMaxThreadCount(2);   // 最多 2 个线程

    QElapsedTimer t;
    t.start();
    for (int i = 0; i < 6; ++i) {
        pool.start(new PoolTask);      // 提交 6 个任务
    }
    pool.waitForDone();

    qDebug() << "  6 个任务、2 个线程耗时" << t.elapsed()
             << "ms（线程复用：任务排队，不创建 6 个线程）";

    return 0;
}
