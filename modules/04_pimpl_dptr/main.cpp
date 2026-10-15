// PImpl / d-pointer 演示
//
// 要点：
//   头文件只暴露接口 + unique_ptr<Impl>，实现细节全在 .cpp。
//   改 Impl 成员 → 头文件不变 → 依赖方不重编译；sizeof 固定为一个指针。

#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include "win_console.h"

#include "media_player.h"

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    qDebug() << "\n--- PImpl / d-pointer ---";

    MediaPlayer player;
    player.open("intro.mp4");
    player.setVolume(0.6);
    player.play();

    qDebug() << "  sizeof(MediaPlayer) =" << sizeof(MediaPlayer)
             << "字节（仅一个指针，实现细节全部隐藏）";

    MediaPlayer copy = player;        // 深拷贝 Impl（完整类型在 .cpp）
    copy.pause();

    MediaPlayer moved = std::move(copy);   // 移动：只搬指针
    qDebug() << "  移动后 volume =" << moved.volume();

    player.pause();
    return 0;
}
