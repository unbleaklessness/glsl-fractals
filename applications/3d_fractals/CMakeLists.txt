find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(PNG REQUIRED)

set(EXECUTABLE 3d_fractals)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp")

add_executable(${EXECUTABLE} ${SOURCES})
target_link_libraries(${EXECUTABLE} PRIVATE glfw)
target_link_libraries(${EXECUTABLE} PRIVATE ${GLEW_LIBRARIES})
target_link_libraries(${EXECUTABLE} PRIVATE ${OPENGL_LIBRARIES})
target_link_libraries(${EXECUTABLE} PRIVATE glm::glm)
target_link_libraries(${EXECUTABLE} PRIVATE ${PNG_LIBRARIES})
target_include_directories(${EXECUTABLE} PRIVATE ${GLEW_INCLUDE_DIRS})
target_include_directories(${EXECUTABLE} PRIVATE ${PNG_INCLUDE_DIRS})
target_compile_options(${EXECUTABLE} PRIVATE -g -O3)
target_compile_features(${EXECUTABLE} PRIVATE cxx_std_17)

set(PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/sources")
set(FILES_TO_COPY
    "${PREFIX}/vertex_shader.vert"
    "${PREFIX}/fragment_shader.frag")

foreach(file ${FILES_TO_COPY})
    add_custom_command(
        TARGET ${EXECUTABLE} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${file}
        "$<TARGET_FILE_DIR:${EXECUTABLE}>")
endforeach()

