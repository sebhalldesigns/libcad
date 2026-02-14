# democad/CMakeLists.txt

add_custom_target(democad_build ALL
    COMMAND cd && npm install && npm run build
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building democad frontend with npm"
)
