#pragma once

#include <QtGlobal>
#include <QString>

#ifdef Q_OS_WIN
#include <cstdio>

// Windows 控制台/IDE 默认按系统 ANSI 代码页（GBK）解码 stderr，
// 而 Qt6 qDebug 默认写 UTF-8 字节 → 中文乱码。
// 在 Windows 下把日志转成本地编码再写 stderr；其他平台为空实现。
// 各模块 main() 开头调用一次：installWinConsoleHandler();
inline void installWinConsoleHandler() {
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& msg) {
        fputs(msg.toLocal8Bit().constData(), stderr);
        fputc('\n', stderr);
        fflush(stderr);
    });
}

#else

inline void installWinConsoleHandler() {}

#endif
