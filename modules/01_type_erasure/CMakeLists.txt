# 模块 01：类型擦除（Type Erasure）
# 两种流派的手写 function + std::function + Qt QMetaType/QVariant 演示
add_executable(type_erasure_demo
        main.cpp
        mini_function.h
        simple_function.h)
target_include_directories(type_erasure_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(type_erasure_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(type_erasure_demo)
