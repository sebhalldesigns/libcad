cmake_minimum_required(VERSION 3.16)

set(CGLM_SHARED OFF CACHE BOOL "Don't build CGLM shared" FORCE)
set(CGLM_STATIC ON CACHE BOOL "Build CGLM static" FORCE)
add_subdirectory(cglm)

set(JANSSON_EXAMPLES OFF CACHE BOOL "Don't build Jansson examples" FORCE)
set(JANSSON_WITHOUT_TESTS ON CACHE BOOL "Don't build Jannson tests" FORCE)
set(JANSSON_BUILD_DOCS OFF CACHE BOOL "Don't build Jannson docs" FORCE)
add_subdirectory(jansson)

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
