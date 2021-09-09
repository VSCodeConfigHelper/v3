cmake_minimum_required(VERSION 3.18.0)

project(vscch3 VERSION 3.1.2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_BUILD_TYPE Debug)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Let MSVC parse source as UTF-8
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Use Unicode Win32 API (-W ver) instead of ANSI
add_definitions(-DUNICODE -D_UNICODE)
# https://github.com/boostorg/process/issues/96 BOOST_USE_WINDOWS_H also
# required by Boost.Nowide
add_definitions(-DBOOST_USE_WINDOWS_H -DWIN32_LEAN_AND_MEAN)
add_definitions(-DJSON_USE_IMPLICIT_CONVERSIONS)

aux_source_directory(src SOURCES)
add_executable(VSCodeConfigHelper ${SOURCES})

# load script files into string
if(WIN32)
  file(READ scripts/pause-console.ps1 PAUSE_CONSOLE_SRC)
  file(READ scripts/check-ascii.ps1 CHECK_ASCII_SRC)
elseif(APPLE)
  file(READ scripts/pause-console.rb PAUSE_CONSOLE_SRC)
  file(READ scripts/pause-console-launcher.sh PAUSE_CONSOLE_LAUNCHER_SRC)
else()
  file(READ scripts/pause-console.sh PAUSE_CONSOLE_SRC)
endif()

configure_file(configs/config.h.in ${CMAKE_BINARY_DIR}/include/config.h)
target_include_directories(VSCodeConfigHelper
                           PRIVATE ${CMAKE_BINARY_DIR}/include)
target_sources(VSCodeConfigHelper PRIVATE configs/resource.rc)
if(MSVC)
  target_sources(VSCodeConfigHelper PRIVATE configs/app.manifest)
endif()

target_link_libraries(VSCodeConfigHelper ${CONAN_LIBS})

# Statically link system runtime library
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_link_libraries(VSCodeConfigHelper -static-libgcc -static-libstdc++)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(VSCodeConfigHelper PRIVATE -static)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set_property(
    TARGET VSCodeConfigHelper PROPERTY MSVC_RUNTIME_LIBRARY
                                       "MultiThreaded$<$<CONFIG:Debug>:Debug>")

endif()

# simplify Linux/Mac output name for convenience
if(NOT WIN32)
  set_property(TARGET VSCodeConfigHelper
               PROPERTY OUTPUT_NAME "vscch")
endif()

# CPack
# `cpack --config build/CPackConfig.cmake`
set(CPACK_GENERATOR 7Z)
set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/package)
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
if(WIN32)
  set(CPACK_SYSTYPE "win")
elseif(APPLE)
  set(CPACK_SYSTYPE "mac")
else()
  set(CPACK_SYSTYPE "linux")
endif()
set(CPACK_PACKAGE_FILE_NAME "VSCodeConfigHelper_v${PROJECT_VERSION}_${CPACK_SYSTYPE}")
install(TARGETS VSCodeConfigHelper RUNTIME DESTINATION .)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE DESTINATION .)
include(CPack)
