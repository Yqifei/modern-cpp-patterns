#pragma once

#include <QString>
#include <QDebug>

// 渲染后端抽象：真实实现与空实现共用同一接口。
// 空对象把"没有后端"建模成一个合法的对象——调用方永远不用判空。

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void drawFrame(int id) = 0;
    virtual void resize(int w, int h) = 0;
    virtual QString name() const = 0;
};

// 空对象：操作全为空，但对象真实存在、可安全调用。
// 单例 + 无状态：所有"空"共享同一实例，静态存储期零分配。
class NullRenderer final : public IRenderer {
public:
    static NullRenderer& instance() {
        static NullRenderer s;
        return s;
    }

    void drawFrame(int) override  {}
    void resize(int, int) override {}
    QString name() const override { return QStringLiteral("null"); }

private:
    NullRenderer() = default;
};

// 真实实现：有实际行为
class OpenGLRenderer final : public IRenderer {
public:
    void drawFrame(int id) override {
        qDebug().noquote() << "    [OpenGLRenderer] 绘制帧" << id;
    }
    void resize(int w, int h) override {
        qDebug().noquote() << "    [OpenGLRenderer] 窗口调整" << w << "x" << h;
    }
    QString name() const override { return QStringLiteral("opengl"); }
};

// 使用方：构造时兜底，之后所有调用零判空
class MediaPipeline {
public:
    MediaPipeline() : renderer_(&NullRenderer::instance()) {}
    explicit MediaPipeline(IRenderer* r)
        : renderer_(r ? r : &NullRenderer::instance()) {}

    void render() { renderer_->drawFrame(++frame_); }
    void resize(int w, int h) { renderer_->resize(w, h); }
    QString backend() const { return renderer_->name(); }

private:
    IRenderer* renderer_ = nullptr;   // 不拥有：生命周期由调用方保证
    int frame_ = 0;
};

// 工厂：未知/禁用后端返回空对象而非 nullptr——返回值永远有效。
IRenderer& createRenderer(const QString& backend) {
    static OpenGLRenderer gl;
    return backend == QStringLiteral("opengl")
               ? static_cast<IRenderer&>(gl)
               : NullRenderer::instance();
}
