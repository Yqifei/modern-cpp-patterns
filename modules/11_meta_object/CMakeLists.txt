# 模块 11：元对象系统（MOC / 反射 / 信号槽底层）
add_executable(meta_object_demo
        main.cpp
        player_profile.h
        player_profile.cpp)
target_include_directories(meta_object_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(meta_object_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(meta_object_demo)
