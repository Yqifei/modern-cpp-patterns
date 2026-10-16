#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>

// MVC 的 Model：数据全在这里。View 只订阅它的信号，从不被直接操作。

class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit PlaylistModel(QObject* parent = nullptr);

    // Qt 框架接口：View 靠这两个函数读数据
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // 业务操作（Controller 只调这些）
    void addTitle();   // 添加一首（演示用：自动取名"曲目N"，省去输入框）
    void next();       // 切到下一首
    QString currentTitle() const;

signals:
    void currentChanged(const QString& title);   // 当前曲目变了，订阅方自己刷新

private:
    QStringList items_;
    int current_ = -1;
};
