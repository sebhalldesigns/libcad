# democad/app/CMakeLists.txt
# This CMakeLists.txt wraps the Cargo build process for the Rust application.

# Define an executable target that Cargo will build.
# We use a custom command to invoke Cargo build and add the ALL keyword
# so that it's built by default with other targets.
add_custom_target(democad_app_rust ALL
    COMMAND cargo build --release
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Rust democad application with Cargo"
)

# Optional: If you want to install the release executable
# install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/target/release/${PROJECT_NAME}.exe
#         DESTINATION bin)

# Optional: If you want to create a dependency on libcad (Rust FFI calls C libcad)
# You would need to make sure libcad is built before this.
# This part will be more complex later when we add Rust bindings to the C library.
# For now, we just build the Rust project.
