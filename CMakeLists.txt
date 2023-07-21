cmake_minimum_required(VERSION 3.20)
project(fractals)
set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

##
## 2D FRACTALS
##

add_executable(2d-fractals 2d_fractals.cpp)

target_link_libraries(2d-fractals PRIVATE glfw ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} glm::glm)
target_include_directories(2d-fractals PRIVATE ${GLEW_INCLUDE_DIRS})

add_custom_command(TARGET 2d-fractals PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/2d_fragment_shader.glsl"
        "$<TARGET_FILE_DIR:2d-fractals>")

add_custom_command(TARGET 2d-fractals PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/2d_vertex_shader.glsl"
        "$<TARGET_FILE_DIR:2d-fractals>")

##
## 3D FRACTALS
##

add_executable(3d-fractals 3d_fractals.cpp)

target_link_libraries(3d-fractals PRIVATE glfw ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} glm::glm)
target_include_directories(3d-fractals PRIVATE ${GLEW_INCLUDE_DIRS})

add_custom_command(TARGET 3d-fractals PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/3d_fragment_shader.glsl"
        "$<TARGET_FILE_DIR:3d-fractals>")

add_custom_command(TARGET 3d-fractals PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/3d_vertex_shader.glsl"
        "$<TARGET_FILE_DIR:2d-fractals>")