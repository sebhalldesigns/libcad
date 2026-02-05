# libcad/tests/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

enable_testing()

add_executable(test_window
    window/main.c
)

target_link_libraries(test_window
    cad-static
)