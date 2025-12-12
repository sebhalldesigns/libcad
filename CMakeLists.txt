cmake_minimum_required(VERSION 3.16)

project(libcad)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Force static CRT on Windows
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MTd")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(extern)
add_subdirectory(democad)

add_library(libcad SHARED
    lib/libcad.c
    lib/lcdraw.cpp
)

if (WIN32)
    set(PLATFORM_LIBS 
        opengl32
    )
else()
    set(PLATFORM_LIBS
        GL
    )
endif()

target_link_libraries(libcad PUBLIC
    ${PLATFORM_LIBS}
    skia
    cglm
    SDL3-static
    cimgui
    imgui_backends
    glad
)

set_target_properties(libcad PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
)

target_include_directories(libcad PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/lib
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/extern
    ${CMAKE_CURRENT_LIST_DIR}/extern/skia
)