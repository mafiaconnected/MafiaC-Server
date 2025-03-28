cmake_minimum_required(VERSION 3.15...3.26)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

project(Server C CXX)

# Embedded pdb in static libraries
set(EMBEDDED_PDB OFF)

# Disable the architecture postfix
set(DISABLE_ARCH_POSTFIX ON)

# Warning level 3
set(CUSTOM_MSVC_WARNING_LEVEL 3)

include(j-common)
include(j-macros)
include(j-dependencies)
include(j-galactic)
include(j-spidermonkey)
include(j-warnings)

include_directories("${DEPENDENCIES_PATH}/include/mongoose6")
include_directories("${DEPENDENCIES_PATH}/include/lua5.3")

if(WIN32)
	add_definitions(-DUNICODE)
	add_definitions(-D_UNICODE)
endif()

add_definitions(-DGALACTIC_OPENSSL=1)

add_definitions(-DCURL_STATICLIB)
add_definitions(-DSDL2_STATIC)

add_definitions(-DENGINE_STATIC)

add_definitions(-DASMJIT_STATIC)

add_subdirectory(Server)
