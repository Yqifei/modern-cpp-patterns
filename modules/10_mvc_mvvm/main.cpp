// 模块 10：MVC / MVP / MVVM —— 同一个「播放器」场景，三种模式各一页
//
// 三个模式的区别只有一个问题：数据变了，界面怎么知道？
//   MVC  = 订阅：View 订阅 Model 信号，框架自动刷新
//   MVP  = 主动推：Presenter 主动调 View 接口（回流路径上没有信号）
//   MVVM = 绑定：ViewModel 发 NOTIFY 信号，谁 connect 谁收到（VM 不认识 View）
//
// 三页行为契约一致（首曲自动成为当前、下一首环绕），差异只在日志措辞：
//   [MVC-View] 订阅收到 / [MVP-View] 被 Presenter 主动调用 / [MVVM-绑定] 收到属性通知
// 每 5 秒定时器模拟"播完自动切歌"的外部事件，观察三页同一时刻的三种日志。

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "win_console.h"

#include "playlist_model.h"
#include "mvp_presenter.h"
#include "view_model.h"

// ---- 三页共用的控件骨架（说明 + 当前标签 + 列表 + 两个按钮） ----
// 三页 UI 完全一样，差别只在"控件怎么接"——这正是三种模式的区别所在。
struct PageUi {
    QLabel* current;
    QPushButton* addBtn;
    QPushButton* nextBtn;
};

static PageUi buildPage(QWidget* page, const QString& tip, QWidget* list) {
    auto* lay = new QVBoxLayout(page);
    lay->addWidget(new QLabel(tip));
    auto* current = new QLabel("当前: (无)");
    lay->addWidget(current);
    lay->addWidget(list);
    auto* row = new QHBoxLayout;
    auto* addBtn = new QPushButton("添加曲目");
    auto* nextBtn = new QPushButton("下一首");
    row->addWidget(addBtn);
    row->addWidget(nextBtn);
    lay->addLayout(row);
    return {current, addBtn, nextBtn};
}

// ---- MVP 页：View 是被动接口，Presenter 主动驱动 ----
class MvpPageWidget : public QWidget, public IPlaylistView {
public:
    explicit MvpPageWidget(QWidget* parent = nullptr)
        : QWidget(parent), presenter_(this) {
        list_ = new QListWidget;
        const PageUi ui = buildPage(this,
            "MVP：按钮转交 Presenter，Presenter 操作后主动调 View 接口刷新", list_);
        current_ = ui.current;
        connect(ui.addBtn, &QPushButton::clicked, this, [this] { presenter_.onAddTitle(); });
        connect(ui.nextBtn, &QPushButton::clicked, this, [this] { presenter_.onNext(); });
        presenter_.init();   // 初始画面也由 Presenter 推送
    }

    // IPlaylistView：由 Presenter 主动驱动（View 自己从不订阅数据）
    void setTitles(const QStringList& titles) override {
        qDebug().noquote() << "  [MVP-View] 被 Presenter 主动调用: setTitles";
        list_->clear();
        list_->addItems(titles);
    }
    void setCurrent(const QString& title) override {
        qDebug().noquote() << "  [MVP-View] 被 Presenter 主动调用: setCurrent(" << title << ")";
        current_->setText("当前: " + title);
    }

    PlaylistPresenter* presenter() { return &presenter_; }

private:
    QListWidget* list_ = nullptr;
    QLabel* current_ = nullptr;
    PlaylistPresenter presenter_;   // 声明在最后：init() 推送时控件已构造完成
};

int main(int argc, char** argv) {
    installWinConsoleHandler();
    QApplication app(argc, argv);
    auto* tabs = new QTabWidget;

    // ---- MVC 页：Controller 只调 Model 方法，View 订阅自动刷新 ----
    auto* mvcModel = new PlaylistModel(tabs);
    auto* mvcView = new QListView;
    mvcView->setModel(mvcModel);   // 订阅：QListView 内部监听 rowsInserted/dataChanged

    auto* mvcPage = new QWidget;
    const PageUi mvcUi = buildPage(mvcPage,
        "MVC：按钮(Controller)只改 Model，View 订阅框架信号自动刷新", mvcView);
    // 当前曲目标签 = 订阅者：收到 Model 信号自己更新
    QObject::connect(mvcModel, &PlaylistModel::currentChanged, mvcPage,
                     [mvcUi](const QString& title) {
                         qDebug().noquote() << "  [MVC-View] 订阅收到 Model 信号: 当前播放" << title;
                         mvcUi.current->setText("当前: " + title);
                     });
    // Controller：按钮只调 Model，从不碰 View
    QObject::connect(mvcUi.addBtn, &QPushButton::clicked, mvcPage, [mvcModel] { mvcModel->addTitle(); });
    QObject::connect(mvcUi.nextBtn, &QPushButton::clicked, mvcPage, [mvcModel] { mvcModel->next(); });

    // ---- MVP 页 ----
    auto* mvpPage = new MvpPageWidget;
    // 想给 MVP 加第二个 View？要改 Presenter：加 View 指针成员 + 每个更新点再调一遍

    // ---- MVVM 页：命令进，绑定出 ----
    auto* vm = new PlaylistViewModel(tabs);
    auto* mvvmList = new QListWidget;
    auto* mvvmPage = new QWidget;
    const PageUi mvvmUi = buildPage(mvvmPage,
        "MVVM：按钮调 ViewModel 命令，NOTIFY 信号让绑定方自动更新", mvvmList);
    // 绑定：View 自己 connect 属性信号，ViewModel 不知道谁在听
    QObject::connect(vm, &PlaylistViewModel::currentTitleChanged, mvvmPage,
                     [mvvmUi](const QString& title) {
                         qDebug().noquote() << "  [MVVM-绑定] 收到属性通知: 当前播放" << title;
                         mvvmUi.current->setText("当前: " + title);
                     });
    QObject::connect(vm, &PlaylistViewModel::titlesChanged, mvvmPage,
                     [mvvmList](const QStringList& titles) {
                         mvvmList->clear();
                         mvvmList->addItems(titles);
                     });
    // 命令：按钮只调命令，不碰数据
    QObject::connect(mvvmUi.addBtn, &QPushButton::clicked, mvvmPage, [vm] { vm->addTrack(); });
    QObject::connect(mvvmUi.nextBtn, &QPushButton::clicked, mvvmPage, [vm] { vm->next(); });
    // 想再加一个 View？只需再 connect 一次，ViewModel 零改动

    // ---- 外部事件：每 5 秒模拟"播完自动切歌"，三页同时收到，观察三种日志 ----
    auto* timer = new QTimer(tabs);
    QObject::connect(timer, &QTimer::timeout, tabs, [mvcModel, mvpPage, vm] {
        mvcModel->next();              // MVC：外部事件 = Controller 调 Model
        mvpPage->presenter()->onNext();// MVP：外部事件直接注入 Presenter
        vm->next();                    // MVVM：外部事件 = 调用命令
    });
    timer->start(5000);

    tabs->addTab(mvcPage, "MVC");
    tabs->addTab(mvpPage, "MVP");
    tabs->addTab(mvvmPage, "MVVM");
    tabs->resize(480, 340);
    tabs->setWindowTitle("MVC / MVP / MVVM 对比演示（点『添加曲目』试试，每 5 秒自动切歌）");
    tabs->show();
    return app.exec();
}
