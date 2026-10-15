// 静态多态（CRTP）与非虚接口（NVI）演示
//
// 场景：
//   A. 帧处理器族 —— CRTP 编译期分派，无 vptr（对照虚函数版）
//   B. 导出任务骨架 —— NVI 外壳固定，前置校验 / 后置清理必然执行

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include "win_console.h"

#include "crtp_processor.h"
#include "nvi_exporter.h"

static void scene_crtp();
static void scene_nvi();

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    scene_crtp();
    scene_nvi();
    return 0;
}

static void scene_crtp() {
    qDebug() << "\n--- CRTP 静态多态 ---";
    qDebug() << "sizeof(GammaProcessor) =" << sizeof(GammaProcessor)
             << "字节（256B 查找表，无 vptr）";
    qDebug() << "sizeof(VirtualProcessor) =" << sizeof(VirtualProcessor)
             << "字节（仅 vptr）";

    GammaProcessor gamma(2.2);
    NoiseReduce denoise;
    QByteArray frame(4096, '\x80');

    gamma.process(frame);     // 编译期分派到 GammaProcessor::processImpl
    denoise.process(frame);   // 编译期分派到 NoiseReduce::processImpl
    qDebug().noquote() << "  kind() =" << GammaProcessor::kind()
                       << "/" << NoiseReduce::kind();
}

static void scene_nvi() {
    qDebug() << "\n--- NVI 非虚接口 ---";
    H264Exporter h264;
    PngSequenceExporter png;

    h264.run("out.mp4");
    png.run("frames/");
}
