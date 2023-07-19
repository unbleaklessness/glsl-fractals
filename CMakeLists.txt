cmake_minimum_required(VERSION 3.20)
project(fractals)
set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

set(EXECUTABLE_NAME ${PROJECT_NAME})
add_executable(${EXECUTABLE_NAME} main.cpp)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE glfw ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} glm::glm)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${GLEW_INCLUDE_DIRS})

add_custom_command(TARGET ${EXECUTABLE_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/fragmentShader.glsl"
        "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>")

add_custom_command(TARGET ${EXECUTABLE_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/vertexShader.glsl"
        "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>")