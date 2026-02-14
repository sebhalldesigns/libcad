# democad/native/CMakeLists.txt

add_custom_target(democad_build_native ALL
    COMMAND cargo build --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building native democad with Tauri"
)
