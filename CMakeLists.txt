cmake_minimum_required(VERSION 3.15)
project(3DSortingVisualizer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(RAYLIB_DIR "C:/raylib-6.0_win64_mingw-w64/raylib-6.0_win64_mingw-w64")

include_directories("${RAYLIB_DIR}/include")
link_directories("${RAYLIB_DIR}/lib")

add_executable(sorting_visualizer main.cpp)

target_link_libraries(sorting_visualizer
    raylib
    opengl32
    gdi32
    winmm
)
