#include "player_profile.h"

PlayerProfile::PlayerProfile(QObject* parent)
    : QObject(parent) {}

QString PlayerProfile::name() const { return name_; }

void PlayerProfile::setName(const QString& name) {
    if (name_ == name) {
        return;
    }
    name_ = name;
    emit nameChanged(name_);   // NOTIFY：属性变化广播
}

int PlayerProfile::level() const { return level_; }

void PlayerProfile::setLevel(int level) {
    if (level_ == level) {
        return;
    }
    level_ = level;
    emit levelChanged(level_);
}

void PlayerProfile::levelUp(int delta) {
    level_ += delta;
    emit levelChanged(level_);
    emit leveledUp(level_);
}

QString PlayerProfile::summary() const {
    return QString("玩家 %1，等级 %2").arg(name_).arg(level_);
}
