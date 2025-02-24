cmake_minimum_required(VERSION 3.1)

project(csat)

set(CMAKE_CXX_STANDARD 17)

# 设置编译器优化选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -g -march=native -flto")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")

# 设置源文件和头文件的路径
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

# 添加头文件
include_directories(src)

# 生成可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})
