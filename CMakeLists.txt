project(damnflags C CXX)
cmake_minimum_required(VERSION 3.6.2)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set (SOURCE_FILES
    src/main.cpp
    src/utils.cpp
    src/compilation_database.cpp
    src/project_structure.cpp)

add_executable(damnflags
    ${SOURCE_FILES})

target_include_directories(damnflags PUBLIC include)
set_property(TARGET damnflags PROPERTY CXX_STANDARD 17)
IF (CMAKE_BUILD_TYPE EQUAL "DEBUG")
    target_compile_options(damnflags PUBLIC -Wall -Wextra -Wpedantic -fsanitize=address)
    target_link_libraries(damnflags PUBLIC asan)
ENDIF()

target_link_libraries(damnflags PUBLIC ${CONAN_LIBS})
target_link_libraries(damnflags PUBLIC stdc++fs)
