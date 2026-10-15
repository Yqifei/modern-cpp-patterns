// 模块 06：空对象（Null Object）
//
// 用"什么也不做的真实对象"替代空指针判空：
//   判空逻辑在构造期兜底一次，之后所有调用点零 if。
//   NullRenderer 是单例无状态对象——空也是一等公民。

#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include "win_console.h"

#include "null_object.h"

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    qDebug() << "\n--- 空对象：判空的替代 ---";

    qDebug() << "  场景1: 默认构造（无后端）→ 直接渲染，静默不崩";
    MediaPipeline silent;
    silent.render();
    silent.resize(1920, 1080);
    qDebug().noquote() << "    backend =" << silent.backend();

    qDebug() << "  场景2: 注入 OpenGL 后端 → 正常绘制";
    OpenGLRenderer gl;
    MediaPipeline withGl(&gl);
    withGl.render();
    withGl.resize(1280, 720);
    qDebug().noquote() << "    backend =" << withGl.backend();

    qDebug() << "  场景3: 工厂返回空对象（未知后端）→ 调用安全";
    IRenderer& backend = createRenderer(QStringLiteral("vulkan"));
    backend.resize(800, 600);      // 空对象：什么都不做
    qDebug().noquote() << "    backend =" << backend.name();

    qDebug() << "  场景4: 工厂返回真实后端";
    IRenderer& real = createRenderer(QStringLiteral("opengl"));
    real.drawFrame(7);

    return 0;
}
