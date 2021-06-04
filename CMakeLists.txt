cmake_minimum_required(VERSION 2.8.12)
project(RaylibGame)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(game ${GAME_ENTRY_FILE})
target_link_libraries(game ${CONAN_LIBS})
