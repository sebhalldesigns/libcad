cmake_minimum_required(VERSION 3.16)


add_subdirectory(cglm)
add_subdirectory(json)

if (EMSCRIPTEN)
    target_compile_options(cglm PRIVATE
        -fPIC
    )
endif()

add_library(glad STATIC
    glad/glad.c
)

target_include_directories(glad PUBLIC
    glad
)   

if (EMSCRIPTEN)
    target_compile_options(glad PRIVATE
        -fPIC
    )
endif()



set(SDL_SHARED OFF CACHE BOOL "Disable shared lib" FORCE) 
set(SDL_STATIC ON CACHE BOOL "Enable static lib" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable test lib" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "Disable X11 test" FORCE)

add_subdirectory(SDL)


add_compile_definitions(CIMGUI_USE_OPENGL3 CIMGUI_USE_SDL3)
set(IMGUI_STATIC ON CACHE BOOL "Enable imgui build" FORCE)

add_subdirectory(cimgui)

add_library(imgui_backends STATIC
    cimgui/imgui/backends/imgui_impl_sdl3.cpp
    cimgui/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_backends PUBLIC
    cimgui/imgui
    SDL/include
)

target_link_libraries(imgui_backends PUBLIC
    cimgui
)

target_compile_definitions(imgui_backends PUBLIC
    -DIMGUI_IMPL_API=extern\ \"C\"
    -DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1
    -DIMGUI_IMPL_OPENGL_LOADER_GL3W
)


