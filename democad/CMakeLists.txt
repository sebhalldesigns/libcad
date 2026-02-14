# democad/CMakeLists.txt

add_custom_target(democad_build ALL
    COMMAND cmd /c "cd /d ${CMAKE_CURRENT_SOURCE_DIR} && npm install && npm run build"
    COMMENT "Building democad frontend with npm"
)
