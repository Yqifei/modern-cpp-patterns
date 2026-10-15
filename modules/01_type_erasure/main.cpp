// 类型擦除演示：手写两种流派的 function、std::function、Qt QMetaType / QVariant
//
// 场景：
//   A. 回调注册表 —— 任意 callable 共存一个容器（非侵入观察者）
//   B. 策略注入 —— std::function 替代策略类族
//   C. 异构值存储 —— std::any / QVariant

#include <QCoreApplication>
#include <QDebug>
#include <QMetaType>
#include <QString>
#include <QVariant>
#include <QtGlobal>

#include <algorithm>
#include <any>
#include <functional>
#include <string>
#include <vector>

#include "win_console.h"

#include "mini_function.h"
#include "simple_function.h"

//------------------------------------------------------------------------------
// 场景 A：回调注册表
//   观察者不再需要继承 Observer 接口：任意 callable 都能注册
//------------------------------------------------------------------------------
class NotificationCenter {
public:
    using Handler = mini_function<void(const QString&)>;

    void subscribe(Handler h) { handlers_.push_back(std::move(h)); }

    void publish(const QString& msg) const {
        for (const auto& h : handlers_)
            if (h) h(msg);
    }

private:
    std::vector<Handler> handlers_;
};

class Player {
public:
    explicit Player(QString name) : name_(std::move(name)) {}
    void onNotify(const QString& from) {
        qDebug().noquote() << "  [Player" << name_ << "] 收到通知:" << from;
    }
private:
    QString name_;
};

static void logAll(const QString& msg) {
    qDebug().noquote() << "  [全局日志] 广播:" << msg;
}

struct LoudEcho {
    void operator()(const QString& msg) const {
        qDebug().noquote() << "  [LoudEcho] !!!" << msg << "!!!";
    }
};

//------------------------------------------------------------------------------
// 场景 B：策略注入
//   策略即 std::function，lambda 即策略，无需 Strategy 类族
//------------------------------------------------------------------------------
using SortStrategy = std::function<void(std::vector<int>&)>;

static void bubbleSort(std::vector<int>& v) {
    for (std::size_t i = 0; i < v.size(); ++i)
        for (std::size_t j = 0; j + 1 < v.size() - i; ++j)
            if (v[j] > v[j + 1]) std::swap(v[j], v[j + 1]);
}

class Sorter {
public:
    void setStrategy(SortStrategy s) { strategy_ = std::move(s); }
    void run(std::vector<int>& v) const {
        if (strategy_) strategy_(v);
        else qDebug() << "  未设置策略";
    }
private:
    SortStrategy strategy_;
};

//------------------------------------------------------------------------------
// 场景 C：异构值存储（std::any / QVariant）
//------------------------------------------------------------------------------
struct Point {
    int x = 0;
    int y = 0;
};
Q_DECLARE_METATYPE(Point)

static void scene_handwritten();
static void scene_std();
static void scene_qt();

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    //scene_handwritten();
    //scene_std();
    scene_qt();
    return 0;
}

//------------------------------------------------------------------------------
// ① 手写实现
//------------------------------------------------------------------------------
static void scene_handwritten() {
    qDebug() << "\n--- 手写两种流派的 function ---";

    qDebug() << "sizeof(mini_function<void(const QString&)>) ="
             << sizeof(mini_function<void(const QString&)>)
             << "字节（操作表指针 8 + 数据指针 8 + SBO 缓冲 16）";
    qDebug() << "sizeof(SimpleFunction<void(const QString&)>) ="
             << sizeof(SimpleFunction<void(const QString&)>)
             << "字节（仅 unique_ptr，无 SBO，一律堆分配）";

    // 流派一：mini_function —— 函数指针表 + SBO
    int base = 100;
    mini_function<int(int)> small = [base](int x) { return x + base; };

    struct BigPayload { char blob[256] = {}; int tag = 7; };
    BigPayload big;
    mini_function<int(int)> large = [big](int x) { return x + big.tag; };

    qDebug() << "  [mini_function] 小 lambda（捕获 int）→ SBO，零堆分配";
    qDebug() << "  [mini_function] 大 lambda（捕获 256B）→ 堆分配";
    qDebug() << "  small(1) =" << small(1) << " large(1) =" << large(1);

    auto copy = small;
    base = 999;
    qDebug() << "  copy(1) =" << copy(1) << "（拷贝后与原对象值隔离）";

    // 流派二：SimpleFunction —— 虚继承 + 堆
    int base2 = 100;
    SimpleFunction<int(int)> sf = [base2](int x) { return x + base2; };
    auto sfCopy = sf;
    base2 = 999;
    qDebug() << "  [SimpleFunction] sf(1) =" << sf(1)
             << " sfCopy(1) =" << sfCopy(1) << "（clone 深拷贝，与原对象隔离）";

    qDebug() << "\n  [场景 A] 回调注册表：四种形态的观察者共存一个容器";
    NotificationCenter center;
    Player jungle("Jungle");

    center.subscribe([&jungle](const QString& m) { jungle.onNotify(m); });
    center.subscribe(&logAll);
    center.subscribe(LoudEcho{});
    center.subscribe([base](const QString& m) {
        Q_UNUSED(base);
        qDebug().noquote() << "  [匿名lambda] 收到:" << m;
    });
    center.publish("游戏开始");

    // 同样的 callable 也能装进 SimpleFunction（虚继承流派）
    SimpleFunction<void(const QString&)> sf2 =
            [&jungle](const QString& m) { jungle.onNotify(m); };
    sf2("SimpleFunction 回调");
}

//------------------------------------------------------------------------------
// ② 标准库
//------------------------------------------------------------------------------
static void scene_std() {
    qDebug() << "\n--- 标准库 std::function  ---";

    qDebug() << "  [场景 B] 策略注入：策略即 std::function";
    Sorter sorter;
    std::vector<int> data{5, 3, 8, 1, 9, 2};

    sorter.setStrategy(bubbleSort);
    std::vector<int> v1 = data;
    sorter.run(v1);
    qDebug() << "    冒泡策略结果:" << v1;

    sorter.setStrategy([](std::vector<int>& v) { std::sort(v.begin(), v.end()); });
    std::vector<int> v2 = data;
    sorter.run(v2);
    qDebug() << "    lambda std::sort 策略结果:" << v2;

    std::any a = 42;
    a = std::string("std::any 可以换类型");
    a = Point{1, 2};
    qDebug() << "  [场景 C 标准库] std::any 当前 type() =" << a.type().name();
    if (auto* p = std::any_cast<Point>(&a))
        qDebug().noquote() << "    any_cast<Point> 成功: (" << p->x << "," << p->y << ")";
    qDebug() << "    any_cast 错误类型:"
             << (std::any_cast<int>(&a) == nullptr ? "指针版返回 nullptr（推荐）"
                                                   : "有值");
}

//------------------------------------------------------------------------------
// ③ Qt 元类型系统
//------------------------------------------------------------------------------
static void scene_qt() {
    qDebug() << "\n--- Qt 元类型系统 QMetaType / QVariant ---";

    QVariant v1 = 42;
    QVariant v2 = QString("Qt string");
    QVariant v3 = QVariant::fromValue(Point{5, 6});

    qDebug().noquote() << "  v1: typeId =" << v1.typeId()
                       << " name =" << v1.metaType().name()
                       << " value =" << v1.toInt();
    qDebug().noquote() << "  v2: typeId =" << v2.typeId()
                       << " name =" << v2.metaType().name()
                       << " value =" << v2.toString();

    qRegisterMetaType<Point>("Point");
    qDebug().noquote() << "  v3: typeId =" << v3.typeId()
                       << " name =" << v3.metaType().name();
    if (v3.canConvert<Point>()) {
        Point p = v3.value<Point>();
        qDebug().noquote() << "    value<Point> 成功: (" << p.x << "," << p.y << ")";
    }

    const QMetaType meta = QMetaType::fromType<Point>();
    void* raw = meta.create();
    qDebug().noquote() << "  QMetaType::create() 按类型信息运行期构造，size ="
                       << meta.sizeOf() << "字节";
    meta.destroy(raw);
}
