cmake_minimum_required(VERSION 3.21.0)

project(vscch3 VERSION 3.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE Debug)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Let MSVC parse source as UTF-8
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Use Unicode Win32 API (-W ver) instead of ANSI
add_definitions(-DUNICODE -D_UNICODE)
# https://github.com/boostorg/process/issues/96
# BOOST_USE_WINDOWS_H also required by Boost.Nowide
add_definitions(-DBOOST_USE_WINDOWS_H -DWIN32_LEAN_AND_MEAN)

aux_source_directory(src SOURCES)
add_executable(VSCodeConfigHelper ${SOURCES})

configure_file(config.h.in ${CMAKE_BINARY_DIR}/include/config.h)
target_include_directories(VSCodeConfigHelper PRIVATE ${CMAKE_BINARY_DIR}/include)

target_link_libraries(VSCodeConfigHelper ${CONAN_LIBS})

# Statically link system runtime library
if(MSYS OR MINGW)
  target_link_libraries(VSCodeConfigHelper -static-libgcc -static-libstdc++)
elseif(MSVC)
  set_property(
    TARGET VSCodeConfigHelper PROPERTY MSVC_RUNTIME_LIBRARY
                                       "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
