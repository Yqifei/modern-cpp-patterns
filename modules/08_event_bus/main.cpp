// 模块 08：EventBus（集中式发布-订阅）
//
// 观察者模式是一对多直连（发布者持有订阅者列表）；
// EventBus 是任意对任意——发布者不认识订阅者，只 publish 事件类型。
// Qt 的信号槽是 per-object 直连的分布式形态；EventBus 是全局解耦的集中形态。

#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <memory>

#include "win_console.h"

#include "event_bus.h"

// 领域事件
struct PlaybackStarted { QString file; };
struct PlaybackProgress { qint64 positionMs; qint64 durationMs; };
struct PlaybackError { QString message; };

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    EventBus bus;

    qDebug() << "\n--- EventBus：集中式发布-订阅 ---";

    qDebug() << "  场景1: 多个订阅者同时收到同一事件";
    auto logTok = bus.subscribe<PlaybackStarted>([](const PlaybackStarted& e) {
        qDebug().noquote() << "    [日志] 播放开始:" << e.file;
    });
    bus.subscribe<PlaybackProgress>([](const PlaybackProgress& e) {
        qDebug().noquote() << "    [日志] 进度" << e.positionMs << "/" << e.durationMs;
    });
    bus.subscribe<PlaybackStarted>([](const PlaybackStarted& e) {
        qDebug().noquote() << "    [UI] 状态栏: 正在播放" << e.file;
    });
    bus.subscribe<PlaybackStarted>([](const PlaybackStarted&) {
        qDebug().noquote() << "    [统计] 播放事件 +1";
    });

    bus.publish(PlaybackStarted{"interview_demo.mp4"});
    qDebug() << "  PlaybackStarted 订阅者数:" << bus.subscriberCount<PlaybackStarted>();
    bus.publish(PlaybackProgress{1200, 60000});

    qDebug() << "  场景2: 退订后不再收到";
    bus.unsubscribe(logTok);
    bus.publish(PlaybackStarted{"second.mp4"});

    qDebug() << "  场景3: 弱订阅——对象销毁后自动跳过";
    {
        auto uiModule = std::make_shared<int>(0);   // 模拟 UI 模块对象
        std::weak_ptr<int> weakGuard = uiModule;    // 总线只持弱引用，不延长生命周期
        bus.subscribeWeak<PlaybackStarted>(weakGuard, [](const PlaybackStarted& e) {
            qDebug().noquote() << "    [弱订阅] 收到" << e.file;
        });
        bus.publish(PlaybackStarted{"alive.mp4"});
    }   // uiModule 在此销毁
    bus.publish(PlaybackStarted{"after_destroy.mp4"});

    qDebug() << "  场景4: 回调中再发布（快照机制，不死锁）";
    bus.subscribe<PlaybackStarted>([&bus](const PlaybackStarted&) {
        qDebug().noquote() << "    [A] 处理开始事件，回调里发布错误事件...";
        bus.publish(PlaybackError{"回调中产生的错误"});
    });
    bus.subscribe<PlaybackError>([](const PlaybackError& e) {
        qDebug().noquote() << "    [B] 错误订阅者收到:" << e.message;
    });
    bus.publish(PlaybackStarted{"chain.mp4"});

    return 0;
}
