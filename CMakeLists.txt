cmake_minimum_required(VERSION 3.10)

# 项目名称
project(ProcessMonitor)

# 设置C++标准为C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
    ./include
)
link_directories(./lib/x64)
# link_directories(./lib/ram)

# 添加可执行文件
add_executable(ProcessMonitor src/main.cpp)
target_link_libraries(ProcessMonitor spdlog pthread)
