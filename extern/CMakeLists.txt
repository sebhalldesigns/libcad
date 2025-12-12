cmake_minimum_required(VERSION 3.16)

set(SDL_SHARED OFF CACHE BOOL "Disable shared lib" FORCE) 
set(SDL_STATIC ON CACHE BOOL "Enable static lib" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Disable test lib" FORCE)
set(SDL_X11_XTEST OFF CACHE BOOL "Disable X11 test" FORCE)

add_subdirectory(SDL)
add_subdirectory(cglm)

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

add_library(glad STATIC
    glad/glad.c
)

target_include_directories(glad PUBLIC
    glad
)   

add_subdirectory(ninja)

set_target_properties(ninja PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/ninja
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/ninja
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/ninja
)

# Ensure Python is available and locate the gn python script directory
find_package(Python3 REQUIRED)

set(SKIA_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/skia")
set(SKIA_BIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/skia/bin")
set(SKIA_BUILD_DIR "${CMAKE_BINARY_DIR}/skia")
set(SKIA_DEPS_CHECK_FILE "${CMAKE_BINARY_DIR}/skia_deps_fetched")
set(SKIA_SPIRV_TOOLS_DIR "${SKIA_SRC_DIR}/third_party/externals/spirv-tools")
set(SKIA_NINJA_TARGET "skia")

if(WIN32)
    set(SKIA_STATIC_LIB "${SKIA_BUILD_DIR}/skia.lib" CACHE FILEPATH "Path to Skia library")
else()
    set(SKIA_STATIC_LIB "${SKIA_BUILD_DIR}/libskia.a" CACHE FILEPATH "Path to Skia library")
endif()

# Detect if Skia dependencies are already present. If not, run git-sync-deps to clone them all.
if (NOT EXISTS "${SKIA_SPIRV_TOOLS_DIR}")
    add_custom_command(
        OUTPUT ${SKIA_DEPS_CHECK_FILE}
        COMMAND ${Python3_EXECUTABLE} tools/git-sync-deps
        COMMAND ${CMAKE_COMMAND} -E touch ${SKIA_DEPS_CHECK_FILE}
        COMMENT "Fetching Skia third-party dependencies"
        WORKING_DIRECTORY ${SKIA_SRC_DIR}
        VERBATIM
    )
else()
    # Create a dummy file so CMake doesn't complain
    file(WRITE ${SKIA_DEPS_CHECK_FILE} "")
endif()

# Fetch GN and Ninja tools
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/gn_fetch
    COMMAND ${Python3_EXECUTABLE} ${SKIA_BIN_DIR}/fetch-gn
    DEPENDS ${SKIA_DEPS_CHECK_FILE} 
    COMMENT "Fetching GN for Skia"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/skia
    VERBATIM
)

# Generate skia code
if (WIN32)
    add_custom_command( 
        OUTPUT ${CMAKE_BINARY_DIR}/skia_gen
        COMMAND ${SKIA_BIN_DIR}/gn gen ${SKIA_BUILD_DIR} --args=is_official_build=false\ is_debug=true\ is_component_build=false\ skia_enable_tools=false\ skia_use_system_libpng=false\ skia_use_system_zlib=false\ skia_use_system_libjpeg_turbo=false\ skia_use_system_harfbuzz=false\ skia_use_system_libwebp=false\ skia_use_system_expat=false\ skia_use_system_icu=false\ skia_use_gl=true\ extra_cflags=[\"/MTd\"]\ extra_ldflags=[\"/MTd\"]
        DEPENDS ${CMAKE_BINARY_DIR}/gn_fetch
        COMMENT "Generating Skia build files"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/skia
        VERBATIM
    )
else()
    add_custom_command( 
        OUTPUT ${CMAKE_BINARY_DIR}/skia_gen
        COMMAND ${SKIA_BIN_DIR}/gn gen ${SKIA_BUILD_DIR} --args=is_official_build=true\ is_component_build=false\ skia_enable_tools=false\ skia_use_system_libpng=false\ skia_use_system_freetype2=false\ skia_use_system_zlib=false\ skia_use_system_libjpeg_turbo=false\ skia_use_system_harfbuzz=false\ skia_use_system_libwebp=false\ skia_use_system_expat=false\ skia_use_system_icu=false\ skia_use_egl=true\ skia_use_gl=true
        DEPENDS ${CMAKE_BINARY_DIR}/gn_fetch
        COMMENT "Generating Skia build files"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/skia
        VERBATIM
    )
endif()

# Build skia
add_custom_command(
    OUTPUT ${SKIA_STATIC_LIB}
    COMMAND ninja -C ${SKIA_BUILD_DIR} skia
    DEPENDS ${CMAKE_BINARY_DIR}/skia_gen
    WORKING_DIRECTORY ${SKIA_BUILD_DIR}
    COMMENT "Building Skia"
    VERBATIM
)

add_custom_target(build_skia
    DEPENDS ${SKIA_STATIC_LIB}
)

add_library(skia STATIC IMPORTED GLOBAL)

set_target_properties(skia PROPERTIES
    IMPORTED_LOCATION "${SKIA_STATIC_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${SKIA_SRC_DIR}/include"
)

add_dependencies(skia build_skia)

