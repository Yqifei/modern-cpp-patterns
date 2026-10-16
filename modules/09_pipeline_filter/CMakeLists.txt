# 模块 09：管道-过滤器（音视频处理链）
add_executable(pipeline_filter_demo
        main.cpp
        pipeline.h)
target_include_directories(pipeline_filter_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(pipeline_filter_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(pipeline_filter_demo)
