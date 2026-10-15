# 模块 03：RAII + Copy-and-Swap（资源管理 / 异常安全赋值）
add_executable(raii_copy_swap_demo
        main.cpp
        raii_scope.h
        copy_swap_buffer.h)
target_include_directories(raii_copy_swap_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(raii_copy_swap_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(raii_copy_swap_demo)
