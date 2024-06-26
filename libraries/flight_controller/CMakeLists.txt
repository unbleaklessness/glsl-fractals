set(LIBRARY flight_controller)

find_package(Eigen3 REQUIRED)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sources/*.cpp)

add_library(${LIBRARY} ${SOURCES})

target_include_directories(
    ${LIBRARY}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/includes>
)
target_include_directories(
    ${LIBRARY}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/includes/${LIBRARY}
)

target_link_libraries(${LIBRARY} PRIVATE Eigen3::Eigen)

target_compile_features(${LIBRARY} PRIVATE cxx_std_17)

set_target_properties(${LIBRARY} PROPERTIES VERSION ${PROJECT_VERSION})

include(GNUInstallDirs)

set(TARGETS ${LIBRARY}_targets)

install(
    TARGETS ${LIBRARY}
    EXPORT ${TARGETS}
    LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT ${TARGETS}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/includes/${LIBRARY}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
