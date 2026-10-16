# 模块 08：EventBus（集中式发布-订阅 / 模块解耦）
add_executable(event_bus_demo
        main.cpp
        event_bus.h)
target_include_directories(event_bus_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(event_bus_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(event_bus_demo)
