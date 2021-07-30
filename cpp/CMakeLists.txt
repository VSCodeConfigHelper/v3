cmake_minimum_required(VERSION 3.21.0)

project(vscch3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

aux_source_directory(src SOURCES)
add_executable(VSCodeConfigHelper src/main.cpp src/logger.h src/logger.cpp)
target_compile_definitions(VSCodeConfigHelper PRIVATE UNICODE _UNICODE)
target_link_libraries(VSCodeConfigHelper ${CONAN_LIBS})
