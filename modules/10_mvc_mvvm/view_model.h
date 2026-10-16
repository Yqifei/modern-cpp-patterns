#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

// MVVM：ViewModel 只暴露「属性 + 命令」，不持有任何 View 指针——
// 谁 connect 它的信号它就通知谁，它自己不知道有几个 View。

class PlaylistViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QStringList titles READ titles NOTIFY titlesChanged)

public:
    explicit PlaylistViewModel(QObject* parent = nullptr);

    QString currentTitle() const;
    QStringList titles() const;

    // 命令：View 点击按钮直接调用
    Q_INVOKABLE void addTrack();   // 添加一首（演示用：自动取名"曲目N"）
    Q_INVOKABLE void next();

signals:
    void currentTitleChanged(const QString& title);
    void titlesChanged(const QStringList& titles);

private:
    QStringList items_;
    int current_ = -1;
};
