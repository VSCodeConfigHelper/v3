cmake_minimum_required(VERSION 3.21.0)

project(vscch3)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE Debug)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

aux_source_directory(src SOURCES)
add_executable(VSCodeConfigHelper ${SOURCES})
target_compile_definitions(VSCodeConfigHelper PRIVATE UNICODE _UNICODE)
target_link_libraries(VSCodeConfigHelper ${CONAN_LIBS})

if(MSYS OR MINGW)
  target_link_libraries(VSCodeConfigHelper -static-libgcc -static-libstdc++)
elseif(MSVC)
  set_property(
    TARGET VSCodeConfigHelper PROPERTY MSVC_RUNTIME_LIBRARY
                                       "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
