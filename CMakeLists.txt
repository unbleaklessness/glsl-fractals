cmake_minimum_required(VERSION 3.22)
project(fractals)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(applications/2d_fractals)
add_subdirectory(applications/3d_fractals)
add_subdirectory(applications/3d_fractals_wallpaper)

