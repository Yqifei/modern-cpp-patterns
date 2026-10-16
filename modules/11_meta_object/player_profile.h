#pragma once

#include <QObject>
#include <QString>

// 元对象系统的演示载体。
//
// Q_OBJECT 宏让 MOC 生成一份静态元对象表（QMetaObject）：
//   - className / superClass：类型与继承链（反射的第一步）
//   - 属性表：Q_PROPERTY 声明的读写/通知描述
//   - 方法表：信号、槽、Q_INVOKABLE 方法（含签名）
// 运行时通过 metaObject() 查这张表，就是 Qt 的反射。

class PlayerProfile : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY levelChanged)

public:
    explicit PlayerProfile(QObject* parent = nullptr);

    QString name() const;
    void setName(const QString& name);

    int level() const;
    void setLevel(int level);

    // Q_INVOKABLE：注册进元对象表，可被 invokeMethod 字符串调用
    Q_INVOKABLE void levelUp(int delta = 1);
    Q_INVOKABLE QString summary() const;

signals:
    void nameChanged(QString name);
    void levelChanged(int level);
    void leveledUp(int newLevel);

private:
    QString name_;
    int level_ = 1;
};
