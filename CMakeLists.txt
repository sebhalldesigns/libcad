
# libcad CMake Build Script 

cmake_minimum_required(VERSION 3.16)

include(ExternalProject)

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

project(libcad)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIBCAD_BUILD_SHARED "Build shared libraries." ON)
option(LIBCAD_BUILD_STATIC "Build static libraries." ON)
option(LIBCAD_BUILD_DEMOCAD "Build the democad web application." ON)
option(LIBCAD_BUILD_DEMOCAD_NATIVE "Build the native democad application." OFF)
option(LIBCAD_BUILD_WASM "Build libcad with Emscripten via ExternalProject." ON)
set(LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE "" CACHE FILEPATH "Path to Emscripten.cmake toolchain file.")
set(LIBCAD_EMSCRIPTEN_NINJA "" CACHE FILEPATH "Path to ninja executable used by the wasm ExternalProject.")

if (WIN32)
    # Force static CRT on Windows
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MT")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

add_subdirectory(extern)

if ((LIBCAD_BUILD_DEMOCAD OR LIBCAD_BUILD_DEMOCAD_NATIVE) AND NOT EMSCRIPTEN)
    add_subdirectory(democad)
endif()

if (LIBCAD_BUILD_DEMOCAD_NATIVE AND NOT EMSCRIPTEN)
    add_subdirectory(democad/native)
endif()

if (LIBCAD_BUILD_WASM AND NOT EMSCRIPTEN)
    if (NOT LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE)
        if (DEFINED ENV{EMSDK})
            set(LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE
                "$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
        endif()
    endif()

    if (LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE AND EXISTS "${LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE}")
        message(STATUS "Emscripten toolchain: ${LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE}")

        set(LIBCAD_WEB_DIST_DIR "${CMAKE_SOURCE_DIR}/democad/dist")
        if (NOT LIBCAD_EMSCRIPTEN_NINJA)
            # Use ninja from PATH by default; callers can still override with
            # -DLIBCAD_EMSCRIPTEN_NINJA=<absolute path>.
            set(LIBCAD_EMSCRIPTEN_NINJA "ninja")
        endif()

        message(STATUS "Emscripten Ninja: ${LIBCAD_EMSCRIPTEN_NINJA}")

        ExternalProject_Add(libcad_wasm
            SOURCE_DIR ${CMAKE_SOURCE_DIR}
            PREFIX "web"
            BINARY_DIR ${CMAKE_BINARY_DIR}/wasm
            CMAKE_GENERATOR Ninja
            BUILD_ALWAYS 1
            INSTALL_COMMAND ""
            CMAKE_ARGS
                -DCMAKE_TOOLCHAIN_FILE=${LIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE}
                -DCMAKE_BUILD_TYPE=Release
                -DCMAKE_C_COMPILER=emcc
                -DCMAKE_CXX_COMPILER=em++
                -DCMAKE_MAKE_PROGRAM=${LIBCAD_EMSCRIPTEN_NINJA}
                -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${LIBCAD_WEB_DIST_DIR}
                -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${LIBCAD_WEB_DIST_DIR}
                -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=${LIBCAD_WEB_DIST_DIR}
                -DLIBCAD_BUILD_WASM=OFF
                -DLIBCAD_BUILD_DEMOCAD=OFF
                -DLIBCAD_BUILD_DEMOCAD_NATIVE=OFF
                -DLIBCAD_BUILD_SHARED=OFF
                -DLIBCAD_BUILD_STATIC=OFF
            BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config Release --target cad
        )
        ExternalProject_Add_Step(libcad_wasm copy_web_outputs
            COMMAND ${CMAKE_COMMAND} -E make_directory ${LIBCAD_WEB_DIST_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/cad.js ${LIBCAD_WEB_DIST_DIR}/cad.js
            COMMAND ${CMAKE_COMMAND} -E copy_if_different <BINARY_DIR>/cad.wasm ${LIBCAD_WEB_DIST_DIR}/cad.wasm
            DEPENDEES build
            ALWAYS 1
        )

        # Ensure frontend dist exists before placing wasm/native artifacts there.
        if (TARGET democad_build)
            add_dependencies(libcad_wasm democad_build)
        endif()
    else()
        message(WARNING
            "LIBCAD_BUILD_WASM is ON, but no valid Emscripten toolchain was found. "
            "Set EMSDK and source emsdk_env, or pass "
            "-DLIBCAD_EMSCRIPTEN_TOOLCHAIN_FILE=<path>/Emscripten.cmake. "
            "Skipping wasm external build target.")
    endif()
endif()

if (WIN32)

    set(PLATFORM_SRC
        lib/render/window/window_win32.c
    )
    
    set(PLATFORM_LIBS 
        opengl32
    )

elseif (EMSCRIPTEN)

    set(PLATFORM_SRC

    )

    set(PLATFORM_LIBS
        "-g"
        "-s USE_WEBGL2=1"
        "-s FULL_ES3=1"
        "-s ALLOW_MEMORY_GROWTH=1"
        "-s USE_ZLIB=1"
        "-s ASSERTIONS=2"
        "-s EXPORTED_RUNTIME_METHODS=['UTF8ToString']"
    )

    set(USE_GLES TRUE)

elseif (APPLE AND IOS)

    set(PLATFORM_SRC

    )
    
    set(PLATFORM_LIBS
        "-framework OpenGLES"
    )

    set(USE_GLES TRUE)

elseif (APPLE)

    set(PLATFORM_SRC

    )
    
    set(PLATFORM_LIBS
        "-framework OpenGL"
    )

else()
    
    set(PLATFORM_SRC

    )
    
    set(PLATFORM_LIBS
        GL
    )

endif()

set(CAD_SRC
    lib/libcad.c
    #lib/lc_draw.c
    #lib/lc_canvas.c
    #lib/lc_scene.c
    lib/render/gpu/gpu.c
    lib/render/vector/vector.c
    lib/util/log/log.c
    lib/model/type/type.c
    lib/types/core/object/object.c
    lib/types/core/document/document.c
    lib/types/core/textfield/textfield.c
    lib/types/core/plane/plane.c
    lib/types/core/axis/axis.c
    lib/types/core/camera/camera.c
    lib/types/sketch/sketch/sketch.c
    lib/types/sketch/line/line.c
    lib/types/sketch/circle/circle.c
    lib/types/sketch/rectangle/rectangle.c
    lib/types/solid/body/body.c
    lib/render/mesh/mesh.c
    ${PLATFORM_SRC}
)

set(CAD_LIBS
    cglm
    glad
    jansson
    ${PLATFORM_LIBS}
)

set(CAD_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/lib
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/extern
)

set(GL_SHADERS
    resources/shaders/vector/line_core_vs.glsl
    resources/shaders/vector/line_core_fs.glsl
    resources/shaders/vector/line_pick_core_vs.glsl
    resources/shaders/vector/line_pick_core_fs.glsl
    resources/shaders/vector/axis_core_vs.glsl
    resources/shaders/vector/axis_core_fs.glsl
    resources/shaders/vector/shape_core_vs.glsl
    resources/shaders/vector/shape_core_fs.glsl
    resources/shaders/vector/shape_pick_core_vs.glsl
    resources/shaders/vector/shape_pick_core_fs.glsl
    resources/shaders/vector/glyph_core_vs.glsl
    resources/shaders/vector/glyph_core_fs.glsl
    resources/shaders/mesh/body_fill_core_vs.glsl
    resources/shaders/mesh/body_fill_core_fs.glsl
    resources/shaders/mesh/body_edge_core_vs.glsl
    resources/shaders/mesh/body_edge_core_fs.glsl
)

set(GLES_SHADERS
    resources/shaders/vector/line_es_vs.glsl
    resources/shaders/vector/line_es_fs.glsl
    resources/shaders/vector/line_pick_es_vs.glsl
    resources/shaders/vector/line_pick_es_fs.glsl
    resources/shaders/vector/axis_es_vs.glsl
    resources/shaders/vector/axis_es_fs.glsl
    resources/shaders/vector/shape_es_vs.glsl
    resources/shaders/vector/shape_es_fs.glsl
    resources/shaders/vector/shape_pick_es_vs.glsl
    resources/shaders/vector/shape_pick_es_fs.glsl
    resources/shaders/vector/glyph_es_vs.glsl
    resources/shaders/vector/glyph_es_fs.glsl
    resources/shaders/mesh/body_fill_es_vs.glsl
    resources/shaders/mesh/body_fill_es_fs.glsl
    resources/shaders/mesh/body_edge_es_vs.glsl
    resources/shaders/mesh/body_edge_es_fs.glsl
)

if(LIBCAD_BUILD_SHARED OR EMSCRIPTEN)

    if (LIBCAD_BUILD_SHARED)
        add_library(cad SHARED
            ${CAD_SRC}
        )
    else()
        add_executable(cad
            ${CAD_SRC}
        )
    endif()

    target_link_libraries(cad PUBLIC
        ${CAD_LIBS}
    )

    target_include_directories(cad PUBLIC
        ${CAD_INCLUDE_DIRS}
    )

    set_target_properties(cad PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
    )

    if (USE_GLES)
        embed_resources(cad
            ${GLES_SHADERS}
        )

        target_compile_definitions(cad PUBLIC USE_GLES)
    else()
        embed_resources(cad
            ${GL_SHADERS}
        )
    endif()
endif()

if(LIBCAD_BUILD_STATIC)
    add_library(cad-static STATIC
        ${CAD_SRC}
    )

    target_link_libraries(cad-static PUBLIC
        ${CAD_LIBS}
    )

    target_include_directories(cad-static PUBLIC
        ${CAD_INCLUDE_DIRS}
    )

    set_target_properties(cad-static PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
    )

    if (USE_GLES)
        embed_resources(cad-static
            ${GLES_SHADERS}
        )

        target_compile_definitions(cad-static PUBLIC USE_GLES)
    else()
        embed_resources(cad-static
            ${GL_SHADERS}
        )
    endif()
endif()

add_subdirectory(tests)
