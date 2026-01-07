# libcad/democad/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

add_executable(democad
    src/main.c
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
        skia
        EGL
        GL
        dl
    )
endif()

target_link_libraries(democad PUBLIC
    SDL3-static
    libcad
    ${PLATFORM_LIBS}
    nlohmann_json
    cglm
    cimgui
    imgui_backends
)

set_target_properties(democad PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
)

target_include_directories(democad PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../extern
)