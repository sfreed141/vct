cmake_minimum_required(VERSION 2.8)

project(voxelize)

file(GLOB_RECURSE SOURCES src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.h)
file(GLOB_RECURSE SHADERS shaders/*.vert shaders/*.frag shaders/*.comp)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SHADERS})
include_directories(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src)

add_definitions(-DRESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources/")
add_definitions(-DSHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW REQUIRED)

include_directories(${PROJECT_NAME} ${GLFW_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${GLEW_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARY} ${OPENGL_gl_LIBRARY} ${GLEW_LIBRARIES})
