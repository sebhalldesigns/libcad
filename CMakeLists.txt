
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
        lib/lcdraw.cpp
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
    

    # Optionally add Emscripten as an ExternalProject
    option(BUILD_WASM_TARGET "Build WebAssembly target" OFF)
    
    if(BUILD_WASM_TARGET)
        include(ExternalProject)
        
        find_program(EMCMAKE emcmake REQUIRED)
        
        ExternalProject_Add(
            libcad_wasm
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
            BINARY_DIR ${CMAKE_BINARY_DIR}/wasm
            CONFIGURE_COMMAND ${EMCMAKE} ${CMAKE_COMMAND}
                -S ${CMAKE_CURRENT_SOURCE_DIR}
                -B <BINARY_DIR>
                -DCMAKE_BUILD_TYPE=Release
                -DEMSCRIPTEN=1
            BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR>
            INSTALL_COMMAND ""
            BUILD_ALWAYS 1
        )

        add_custom_target(build_wasm ALL DEPENDS libcad_wasm)

        # Make your main target depend on the WASM build
        add_dependencies(libcad libcad_wasm)
    endif()

else()

    add_subdirectory(extern)

    add_executable(cad_wasm
        lib/libcad.c
        lib/lcdraw.cpp  
    )

    target_include_directories(cad_wasm PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/lib
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/extern
        ${CMAKE_CURRENT_LIST_DIR}/extern/skia
        ${CMAKE_CURRENT_LIST_DIR}/extern/SDL/include
    )


    target_link_libraries(cad_wasm PUBLIC
        cglm
        "-Wl,--whole-archive" skia "-Wl,--no-whole-archive"
        glad
    )

    set_target_properties(cad_wasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}
        SUFFIX ".wasm"
        PREFIX "lib"
    )

    target_compile_options(cad_wasm PRIVATE
        -sSIDE_MODULE=1
        -sUSE_SDL=3
    )


    target_link_options(cad_wasm PUBLIC
        -sUSE_SDL=3
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sALLOW_MEMORY_GROWTH=1
        -sMAX_WEBGL_VERSION=2
        -sSIDE_MODULE=1
    )

endif()