// 模块 11：元对象系统（Meta-Object System）
//
// 场景1: MOC 产物——className 与继承链
// 场景2: 元对象表——属性表 + 方法表（MOC 生成的静态反射数据）
// 场景3: 属性系统——Q_PROPERTY 动态读写 + 动态属性
// 场景4: 信号槽——编译期 connect + emit 派发 + NOTIFY
// 场景5: 反射调用——QMetaObject::invokeMethod 字符串寻址

#include <QCoreApplication>
#include <QDebug>
#include <QMetaMethod>
#include <QMetaObject>
#include <QMetaProperty>
#include <QObject>

#include "win_console.h"

#include "player_profile.h"

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QCoreApplication app(argc, argv);

    PlayerProfile profile;

    qDebug() << "\n--- 元对象系统 ---";

    qDebug() << "  1) MOC 产物：className 与继承链";
    const auto* meta = profile.metaObject();
    qDebug().noquote() << "     className   =" << meta->className();
    qDebug().noquote() << "     superClass  =" << meta->superClass()->className();

    qDebug() << "  2) 元对象表（MOC 静态生成，运行时查表 = 反射）";
    qDebug().noquote() << "     属性表（自声明"
                       << (meta->propertyCount() - meta->propertyOffset()) << "个）";
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i) {
        const auto p = meta->property(i);
        qDebug().noquote() << "       " << p.name() << ":" << p.typeName()
                           << (p.isWritable() ? "可写" : "只读")
                           << (p.hasNotifySignal() ? "+NOTIFY" : "");
    }
    qDebug().noquote() << "     方法表（自声明"
                       << (meta->methodCount() - meta->methodOffset()) << "个）";
    for (int i = meta->methodOffset(); i < meta->methodCount(); ++i) {
        const auto m = meta->method(i);
        const char* kind = m.methodType() == QMetaMethod::Signal ? "信号"
                           : m.methodType() == QMetaMethod::Slot ? "槽"
                                                                 : "方法";
        qDebug().noquote() << "       [" << kind << "]" << m.methodSignature();
    }

    qDebug() << "  3) 属性系统：Q_PROPERTY 动态读写 + 动态属性";
    profile.setProperty("name", "Jungle");     // 走 WRITE setName
    profile.setProperty("level", 12);          // 走 WRITE setLevel
    qDebug().noquote() << "     property(name)  =" << profile.property("name").toString();
    qDebug().noquote() << "     property(level) =" << profile.property("level").toInt();
    profile.setProperty("vip", true);          // 动态属性：无需 Q_PROPERTY 声明
    qDebug().noquote() << "     动态属性 vip =" << profile.property("vip").toBool();

    qDebug() << "  4) 信号槽：connect + emit（编译期绑定）+ NOTIFY";
    QObject::connect(&profile, &PlayerProfile::leveledUp, &app,
                     [](int lv) {
                         qDebug().noquote() << "     [槽] 收到 leveledUp:" << lv;
                     });
    QObject::connect(&profile, &PlayerProfile::nameChanged, &app,
                     [](const QString& n) {
                         qDebug().noquote() << "     [NOTIFY] nameChanged →" << n;
                     });
    profile.levelUp(1);                        // emit leveledUp
    profile.setName("Alice");                  // emit nameChanged

    qDebug() << "  5) 反射调用：QMetaObject::invokeMethod（字符串寻址）";
    QMetaObject::invokeMethod(&profile, "levelUp", Q_ARG(int, 2));
    qDebug().noquote() << "     invokeMethod(levelUp,2) 后 level =" << profile.level();
    QString summary;
    QMetaObject::invokeMethod(&profile, "summary", Q_RETURN_ARG(QString, summary));
    qDebug().noquote() << "     invokeMethod(summary) 返回:" << summary;

    return 0;
}
