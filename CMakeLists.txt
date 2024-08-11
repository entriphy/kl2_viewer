cmake_minimum_required(VERSION 3.5.0)
project(kl2_renderer VERSION 0.1.0 LANGUAGES C CXX)

set(COMMON_SRC src/vu0.c src/gs.c src/gms.c src/romfs.cpp)
add_executable(kl2_renderer
    src/main.c
    ${COMMON_SRC}

    src/take/camera.c
    src/take/mimesys.c
    src/take/motip.c
    src/take/motsys.c
    src/take/motsys2.c
    src/take/object.c
    src/take/sfxbios.c

    lib/cimgui/imgui/backends/imgui_impl_opengl3.cpp
    lib/cimgui/imgui/backends/imgui_impl_glfw.cpp
)
target_include_directories(kl2_renderer PRIVATE src include lib/cimgui/generator/output)
target_compile_options(kl2_renderer PUBLIC -msse)
set_property(TARGET kl2_renderer PROPERTY C_STANDARD 99)
set_property(TARGET kl2_renderer PROPERTY CXX_STANDARD 20) # Must be 20 for libromfs

# Check if game files have been extracted
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/res/spec.gim)
    message(FATAL_ERROR "res/spec.gim does not exist. Please run the extract script first.")
endif()

# add_library(kl2_lib SHARED ${COMMON_SRC})
# target_include_directories(kl2_lib PRIVATE src include)
# target_compile_options(kl2_lib PUBLIC -msse)
# set_property(TARGET kl2_lib PROPERTY C_STANDARD 99)
# set_property(TARGET kl2_lib PROPERTY CXX_STANDARD 20) # Must be 20 for libromfs

# Libraries
set(OpenGL_GL_PREFERENCE GLVND)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LIBROMFS_PROJECT_NAME ${PROJECT_NAME})
set(LIBROMFS_RESOURCE_LOCATION ${CMAKE_SOURCE_DIR}/res)

add_subdirectory(lib/cglm)
add_subdirectory(lib/cimgui)
add_subdirectory(lib/glew)
add_subdirectory(lib/glfw)
add_subdirectory(lib/libromfs)

# Link
find_package(OpenGL REQUIRED)
target_link_libraries(kl2_renderer PUBLIC m
    OpenGL::GL
    OpenGL::EGL
    GLEW
    glfw
    cglm
    cimgui
    ${LIBROMFS_LIBRARY}
)