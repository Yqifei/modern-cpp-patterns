// 模块 05：SFINAE + 移动语义
//
// 两个话题在这里合流：
//   SFINAE   —— 编译期按"类型能力"选路（has_to_string 探测 / enable_if 重载）
//   移动语义 —— 运行期零拷贝搬移（MoveBuffer 独占堆缓冲 / vector 扩容）

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <vector>

#include "win_console.h"

#include "move_aware_buffer.h"
#include "sfinae_helpers.h"

// 带 toString() 的领域类型：stringify 自动走 toString 分支
struct Version {
    int major;
    int minor;
    std::string toString() const {
        return std::to_string(major) + "." + std::to_string(minor);
    }
};

// 带移动/拷贝计数的探测类型：证明 vector 扩容走了哪条路
struct TraceMove {
    static inline int moves = 0;
    static inline int copies = 0;
    TraceMove() = default;
    TraceMove(const TraceMove&) { ++copies; }
    TraceMove(TraceMove&&) noexcept { ++moves; }
};

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    qDebug() << "\n--- SFINAE：按类型能力选路 ---";
    qDebug().noquote() << "  int     ->" << QString::fromStdString(sfinae::stringify(42));
    qDebug().noquote() << "  double  ->" << QString::fromStdString(sfinae::stringify(3.14));
    qDebug().noquote() << "  Version ->" << QString::fromStdString(sfinae::stringify(Version{2, 7}));
    qDebug().noquote() << "  has_to_string_v<int> = " << sfinae::has_to_string_v<int>
                       << "  has_to_string_v<Version> = " << sfinae::has_to_string_v<Version>;
    qDebug().noquote() << "  describe20(7) ->" << QString::fromStdString(sfinae::describe20(7));

    qDebug() << "\n--- 移动语义：拷贝 vs 移动的内存路径 ---";
    MoveBuffer big(64 * 1024);   // 64KB
    qDebug() << "  sizeof(MoveBuffer) =" << sizeof(MoveBuffer) << "字节（size/cap/ptr × 8）";

    MoveBuffer copy = big;                    // 深拷贝：堆上新分配一份
    qDebug().noquote() << "  拷贝: big.data = " << static_cast<const void*>(big.data())
                       << "  copy.data = " << static_cast<const void*>(copy.data())
                       << "（地址不同 = 两份数据）";

    MoveBuffer moved = std::move(copy);       // 移动：搬指针，零分配
    qDebug().noquote() << "  移动: moved.data = " << static_cast<const void*>(moved.data())
                       << "  copy.size = " << copy.size() << "（源已置空）";

    // vector 扩容自动走 noexcept 移动（而非拷贝）：计数器证明
    std::vector<TraceMove> traced;
    traced.reserve(1);
    traced.emplace_back();
    traced.emplace_back();   // 扩容 → 搬移已有元素
    traced.emplace_back();   // 再扩容 → 搬移两个元素
    qDebug().noquote() << "  扩容: 移动构造 " << TraceMove::moves << " 次, 拷贝构造 "
                       << TraceMove::copies << " 次（noexcept 移动被选中，拷贝 0 次）";

    qDebug() << "\n--- 完美转发：值类别保持 ---";
    int x = 1;
    qDebug().noquote() << "  relay(x)   ->" << QString::fromStdString(sfinae::classify(x));
    qDebug().noquote() << "  relay(1)   ->" << QString::fromStdString(sfinae::classify(1));

    return 0;
}
