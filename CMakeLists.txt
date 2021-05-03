cmake_minimum_required(VERSION 2.8.12)
project(RaylibGame)

add_definitions("-std=c++17")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(game ${GAME_ENTRY_FILE})
target_link_libraries(game ${CONAN_LIBS})
