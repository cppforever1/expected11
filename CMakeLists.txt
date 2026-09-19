cmake_minimum_required(VERSION 3.10)
project(expected11 LANGUAGES CXX)

add_library(expected11 INTERFACE)
target_include_directories(expected11 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(expected11 INTERFACE cxx_std_11)

enable_testing()
add_executable(expected11_tests tests/expected11_tests.cpp)
target_link_libraries(expected11_tests PRIVATE expected11)
target_compile_options(expected11_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME expected11_tests COMMAND expected11_tests)
