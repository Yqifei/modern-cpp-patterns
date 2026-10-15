# 模块 05：SFINAE + 移动语义（编译期选路 / 零拷贝搬移）
add_executable(sfinae_move_demo
        main.cpp
        sfinae_helpers.h
        move_aware_buffer.h)
target_include_directories(sfinae_move_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(sfinae_move_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(sfinae_move_demo)
