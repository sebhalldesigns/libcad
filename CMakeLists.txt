
# libcad CMake Build Script 

cmake_minimum_required(VERSION 3.16)

project(libcad)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    # Force static CRT on Windows
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

if (NOT EMSCRIPTEN)

    add_subdirectory(extern)
    add_subdirectory(democad)

    add_library(libcad STATIC
        lib/libcad.c
        lib/lc_draw.c
        lib/lc_canvas.c
    )

    if (WIN32)
        set(PLATFORM_LIBS 
            opengl32
        )
    elseif (APPLE)
        set(PLATFORM_LIBS
            "-framework OpenGL"
        )
    else()
        set(PLATFORM_LIBS
            GL
        )
    endif()

    target_link_libraries(libcad PUBLIC
        ${PLATFORM_LIBS}
        cglm
        SDL3-static
        cimgui
        imgui_backends
        glad
        jansson
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
    )

else()

    add_subdirectory(extern)
    add_subdirectory(democad)

    add_library(libcad STATIC
        lib/libcad.c
        lib/lc_draw.c
        lib/lc_canvas.c
    )

    set(PLATFORM_LIBS
        "-s USE_WEBGL2=1"
        "-s FULL_ES3=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s USE_SDL=3"
        "-s USE_ZLIB=1"
    )

    target_link_libraries(libcad PUBLIC
        ${PLATFORM_LIBS}
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
    )
endif()