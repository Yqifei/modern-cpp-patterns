# 模块 04：PImpl / d-pointer（编译隔离 / ABI 稳定 / 隐藏实现）
add_executable(pimpl_dptr_demo
        main.cpp
        media_player.cpp
        media_player.h)
target_include_directories(pimpl_dptr_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(pimpl_dptr_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(pimpl_dptr_demo)
