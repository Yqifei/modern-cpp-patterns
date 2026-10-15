#pragma once

#include <QDebug>
#include <QString>

// NVI（Non-Virtual Interface）：
// public 接口全部非虚（外壳），虚函数收进 protected（实现）。
// 骨架固定：前置校验、后置清理必然执行，子类只能覆写虚实现，无法绕过公共逻辑。
//
// 与模板方法的区别：模板方法是"算法骨架 + 可覆写步骤"；
// NVI 强调"接口非虚"——外壳即契约，公共前置/后置处理不允许子类跳过。
class ExportJob {
public:
    // 非虚外壳：固定流程，子类不能修改
    bool run(const QString& outPath) {
        if (!prepare(outPath)) {
            qDebug().noquote() << QString("  [%1] 前置校验失败，拒绝执行").arg(name());
            return false;
        }
        const bool ok = doRun(outPath);   // 虚实现：子类唯一入口
        finish(ok);                       // 后置：无论成败都执行
        return ok;
    }

    QString name() const { return nameImpl(); }

    virtual ~ExportJob() = default;

protected:
    // 子类覆写点（protected，不对外暴露）
    virtual bool doRun(const QString& outPath) = 0;
    virtual QString nameImpl() const = 0;
    virtual void finish(bool ok) { Q_UNUSED(ok); }

private:
    // 公共前置逻辑：非虚，子类碰不到
    bool prepare(const QString& outPath) {
        return !outPath.isEmpty();
    }
};

class H264Exporter : public ExportJob {
protected:
    bool doRun(const QString& outPath) override {
        qDebug().noquote() << "  [H264] 编码写盘:" << outPath;
        return true;
    }
    QString nameImpl() const override { return QStringLiteral("H264"); }
};

class PngSequenceExporter : public ExportJob {
protected:
    bool doRun(const QString& outPath) override {
        qDebug().noquote() << "  [PNG] 逐帧写序列:" << outPath;
        return false;   // 模拟失败，验证后置清理仍执行
    }
    QString nameImpl() const override { return QStringLiteral("PNG序列"); }
    void finish(bool ok) override {
        qDebug().noquote() << "  [PNG] 后置清理（即使失败也执行，ok =" << ok << "）";
    }
};
