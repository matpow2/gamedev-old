cmake_minimum_required (VERSION 2.6)
project(Chowdren)

add_executable(Chowdren run.cpp)
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}")
find_library(GLFW_LIBRARY GLFW lib)
find_library(SOIL_LIBRARY SOIL lib)
find_library(FT_LIBRARY freetype lib)
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})
target_link_libraries(Chowdren ${GLFW_LIBRARY} 
    ${SOIL_LIBRARY} ${FT_LIBRARY} ${OPENGL_LIBRARIES})