cmake_minimum_required(VERSION 2.8)

project(vct)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)

file(GLOB_RECURSE SOURCES src/*.cpp src/*.c ext/src/*.c ext/src/*.cpp)
file(GLOB_RECURSE HEADERS src/*.hpp src/*.h ext/include/*.h ext/include/*.hpp)
file(GLOB_RECURSE SHADERS shaders/*.vert shaders/*.frag shaders/*.comp)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${SHADERS})
include_directories(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/ext/include)

add_definitions(-DRESOURCE_DIR="${CMAKE_SOURCE_DIR}/resources/")
add_definitions(-DSHADER_DIR="${CMAKE_SOURCE_DIR}/shaders/")

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

include_directories(${PROJECT_NAME} ${OPENGL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} glfw ${OPENGL_gl_LIBRARY})

if(NOT WIN32)
    target_link_libraries(${PROJECT_NAME} dl)
endif()

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()