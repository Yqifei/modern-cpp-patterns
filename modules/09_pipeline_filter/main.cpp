// 模块 09：管道-过滤器
//
// 场景1: 接口版音频处理链（低通 → 增益 → 静音门限 → 峰值统计）
// 场景2: 函数式 Pipeline（lambda 直接组合，零继承）

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "win_console.h"

#include "pipeline.h"

// 生成测试信号：1kHz 正弦 + 伪随机噪声（固定 seed，结果可复现）
static std::vector<float> genTone(std::size_t n, float noiseAmp) {
    std::vector<float> s(n);
    std::uint32_t seed = 12345u;
    for (std::size_t i = 0; i < n; ++i) {
        float tone = 0.5f * std::sin(2 * 3.14159f * 1000.0f * i / 44100.0f);
        seed = seed * 1103515245u + 12345u;
        float noise = (static_cast<float>((seed >> 16) & 0x7fff) / 32767.0f) - 0.5f;
        s[i] = tone + noiseAmp * noise;
    }
    return s;
}

static float peakOf(const std::vector<float>& v) {
    return *std::max_element(v.begin(), v.end());
}

static float rmsOf(const std::vector<float>& v) {
    float sum = 0.0f;
    for (float x : v) sum += x * x;
    return std::sqrt(sum / static_cast<float>(v.size()));
}

// ---- 接口版过滤器 ----

// 低通：一阶 IIR，y[i] = a*x[i] + (1-a)*y[i-1]
class LowPassFilter : public IFilter<std::vector<float>> {
public:
    explicit LowPassFilter(float alpha) : m_alpha(alpha) {}

    std::vector<float> process(std::vector<float> data) override {
        ++m_frames;
        float prev = 0.0f;
        for (float& v : data) {
            v = m_alpha * v + (1.0f - m_alpha) * prev;
            prev = v;
        }
        return data;
    }

    const char* name() const override { return "低通滤波"; }

private:
    float m_alpha;
};

// 增益
class GainFilter : public IFilter<std::vector<float>> {
public:
    explicit GainFilter(float gain) : m_gain(gain) {}

    std::vector<float> process(std::vector<float> data) override {
        ++m_frames;
        for (float& v : data) v *= m_gain;
        return data;
    }

    const char* name() const override { return "音量增益"; }

private:
    float m_gain;
};

// 静音门限：RMS 低于阈值的帧整体置零
class NoiseGateFilter : public IFilter<std::vector<float>> {
public:
    explicit NoiseGateFilter(float threshold) : m_threshold(threshold) {}

    std::vector<float> process(std::vector<float> data) override {
        ++m_frames;
        if (rmsOf(data) < m_threshold) {
            std::fill(data.begin(), data.end(), 0.0f);
        }
        return data;
    }

    const char* name() const override { return "静音门限"; }

private:
    float m_threshold;
};

// 峰值统计（链终点）
class PeakStatsFilter : public IFilter<std::vector<float>> {
public:
    std::vector<float> process(std::vector<float> data) override {
        ++m_frames;
        m_peak = std::max(m_peak, peakOf(data));
        return data;
    }

    const char* name() const override { return "峰值统计"; }
    float peak() const { return m_peak; }

private:
    float m_peak = 0.0f;
};

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    qDebug() << "\n--- 管道-过滤器 ---";

    qDebug() << "  1) 音频处理链（低通→增益→静音门限→峰值统计）";
    auto signal = genTone(44100, 0.3f);   // 1 秒信号（44100 采样）
    qDebug().noquote() << "     原始信号: 峰值" << peakOf(signal)
                       << " RMS" << rmsOf(signal);

    LowPassFilter lp(0.15f);
    GainFilter gain(2.0f);
    NoiseGateFilter gate(0.02f);
    PeakStatsFilter peak;

    FilterChain<std::vector<float>> chain;
    chain.add(&lp).add(&gain).add(&gate).add(&peak);

    QElapsedTimer t;
    t.start();
    auto out = chain.process(signal);
    qDebug().noquote() << "     处理后:   峰值" << peak.peak()
                       << " RMS" << rmsOf(out)
                       << " 耗时" << t.elapsed() << "ms";
    qDebug().noquote() << "     各过滤器处理帧数:"
                       << lp.frames() << "/" << gain.frames()
                       << "/" << gate.frames() << "/" << peak.frames();

    qDebug() << "  2) 函数式 Pipeline（std::function 组合）";
    Pipeline<std::vector<float>> fp;
    fp.add([](std::vector<float> v) {           // 归一化到 [-1, 1]
               for (float& x : v) x /= 32768.0f;
               return v;
           })
      .add([](std::vector<float> v) {           // 软削波限幅
               for (float& x : v) x = std::clamp(x, -1.0f, 1.0f);
               return v;
           })
      .add([](std::vector<float> v) {           // 半音量
               for (float& x : v) x *= 0.5f;
               return v;
           });
    auto routed = fp.run(signal);
    qDebug().noquote() << "     过滤器数量:" << fp.size()
                       << " 输出峰值:" << peakOf(routed);

    return 0;
}
