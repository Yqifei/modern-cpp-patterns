# 模块 06：空对象（Null Object）——用空行为替代空指针判空
add_executable(null_object_demo
        main.cpp
        null_object.h)
target_include_directories(null_object_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(null_object_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(null_object_demo)
