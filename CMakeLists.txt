
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
    #add_subdirectory(democad)

    add_library(cad SHARED
        lib/libcad.c
        lib/lc_draw.c
        lib/lc_canvas.c
        lib/lc_scene.c
    )

    if (WIN32)
        set(PLATFORM_LIBS 
            opengl32
        )
    elseif (APPLE AND IOS)
        set(PLATFORM_LIBS
            "-framework OpenGLES"
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

    target_link_libraries(cad PUBLIC
        ${PLATFORM_LIBS}
        cglm
        #SDL3-static
        cimgui
        imgui_backends
        glad
        jansson
    )

    set_target_properties(cad PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
    )

    target_include_directories(cad PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/extern
    )

else()

    add_subdirectory(extern)
    #add_subdirectory(democad)

    add_library(cad STATIC
        lib/libcad.c
        lib/lc_draw.c
        lib/lc_canvas.c
        lib/lc_scene.c
    )

    set(PLATFORM_LIBS
        "-s USE_WEBGL2=1"
        "-s FULL_ES3=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s USE_SDL=3"
        "-s USE_ZLIB=1"
    )

    target_link_libraries(cad PUBLIC
        ${PLATFORM_LIBS}
        cglm
        SDL3-static
        cimgui
        imgui_backends
        glad
        jansson
    )

    set_target_properties(cad PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
    )

    target_include_directories(cad PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/extern
    )
endif()

# Embed Resources function courtesy of shir0areed on GitHub
function(embed_resources target)
    set(script_path "${CMAKE_CURRENT_BINARY_DIR}/anything_to_c.cmake")
    file(WRITE  "${script_path}" "file(READ \${CMAKE_ARGV3} buf HEX)\n")
    file(APPEND "${script_path}" "string(REGEX REPLACE \"([0-9a-f][0-9a-f])\" \"0x\\\\1, \" buf \${buf})\n")
    file(APPEND "${script_path}" "file(WRITE \${CMAKE_ARGV4} \"const unsigned char \${CMAKE_ARGV5}[] = { \${buf}0x00 };\\n\")\n")
    file(APPEND "${script_path}" "file(APPEND \${CMAKE_ARGV4} \"const unsigned \${CMAKE_ARGV6} = sizeof(\${CMAKE_ARGV5}) - 1;\\n\")\n")
    foreach(res_path ${ARGN})
        string(MAKE_C_IDENTIFIER ${res_path} identifier)
        set(src_path "${CMAKE_CURRENT_SOURCE_DIR}/${res_path}")
        set(dst_path "${CMAKE_CURRENT_BINARY_DIR}/${identifier}.c")
        set(anything_to_c ${CMAKE_COMMAND} -P ${script_path} ${src_path} ${dst_path} ${identifier} ${identifier}_size)
        add_custom_command(OUTPUT ${dst_path} COMMAND ${anything_to_c} DEPENDS ${src_path} VERBATIM)
        target_sources(${target} PRIVATE ${src_path} ${dst_path} )
    endforeach()
endfunction()

if (NOT EMSCRIPTEN AND NOT IOS)
    embed_resources(cad
        resources/shaders/basic/basic_core_vs.glsl
        resources/shaders/basic/basic_core_fs.glsl
        resources/shaders/vector/vector_core_vs.glsl
        resources/shaders/vector/vector_core_fs.glsl
    )
else()
    embed_resources(cad
        resources/shaders/basic/basic_es_vs.glsl
        resources/shaders/basic/basic_es_fs.glsl
    )
endif()
