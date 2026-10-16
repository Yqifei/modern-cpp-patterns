# 模块 10：MVC / MVP / MVVM（Qt GUI 三页签对比演示）
add_executable(mvc_mvvm_demo
        main.cpp
        playlist_model.h
        playlist_model.cpp
        mvp_presenter.h
        mvp_presenter.cpp
        view_model.h
        view_model.cpp)
target_include_directories(mvc_mvvm_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(mvc_mvvm_demo PRIVATE Qt6::Core Qt6::Widgets)
designcode_deploy_runtime(mvc_mvvm_demo)
