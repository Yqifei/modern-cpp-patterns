# 模块 02：CRTP + NVI（静态多态 / 非虚接口）
add_executable(crtp_nvi_demo
        main.cpp
        crtp_processor.h
        nvi_exporter.h)
target_include_directories(crtp_nvi_demo PRIVATE ${CMAKE_SOURCE_DIR}/common)
target_link_libraries(crtp_nvi_demo PRIVATE Qt6::Core)
designcode_deploy_runtime(crtp_nvi_demo)
