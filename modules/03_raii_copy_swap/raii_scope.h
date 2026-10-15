#pragma once

#include <cstdio>
#include <mutex>

#include <QDebug>

// RAII（Resource Acquisition Is Initialization）：
// 资源生命周期绑定对象生命周期——构造获取资源，析构释放资源。
// 栈展开时析构必然执行，因此无论正常返回还是异常路径，资源都一定被释放。
// 对比手动管理：手动 close/unlock 一旦漏在异常路径之前，资源就泄漏。

// 文件句柄：构造打开，析构关闭
class FileHandle {
public:
    explicit FileHandle(const char* path) {
        f_ = std::fopen(path, "wb");
        if (f_) qDebug().noquote() << "  打开文件:" << path;
        else qDebug().noquote() << "  打开失败:" << path;
    }

    ~FileHandle() {
        if (f_) {
            std::fclose(f_);
            qDebug() << "  关闭文件（析构释放）";
        }
    }

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    void write(const char* bytes, std::size_t n) {
        if (f_) std::fwrite(bytes, 1, n, f_);
    }

private:
    std::FILE* f_;
};

// 作用域锁：构造加锁，析构解锁（std::lock_guard 的自制简化版）
class ScopedLock {
public:
    explicit ScopedLock(std::mutex& m) : m_(m) { m_.lock(); }
    ~ScopedLock() { m_.unlock(); }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    std::mutex& m_;
};
