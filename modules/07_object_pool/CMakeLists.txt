# 模块 07：对象池 + QThreadPool（实例复用 / 线程复用）
add_executable(object_pool_demo
        main.cpp
        object_pool.h)
target_include_directories(object_pool_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(object_pool_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(object_pool_demo)
