cmake_minimum_required(VERSION 3.13)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR};${CMAKE_SOURCE_DIR}/cmake/")

project(Server C CXX)

# Embedded pdb in static libraries
set(EMBEDDED_PDB OFF)

# Disable the architecture postfix
set(DISABLE_ARCH_POSTFIX ON)

# Warning level 4
set(CUSTOM_MSVC_WARNING_LEVEL 4)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include(j-common)
include(j-macros)
include(j-dependencies)
include(j-galactic)
include(j-spidermonkey)
include(j-warnings)

include_directories("${DEPENDENCIES_PATH}/include/mongoose6")
include_directories("${DEPENDENCIES_PATH}/include/lua5.3")

if(MSVC)
	if(MSVC_VERSION GREATER_EQUAL 1900)
		# Enable Additional Security Checks
		#add_compiler_flags("/sdl")
	endif()

	# export 'exportname' specified multiple times; using first specification
	disable_linker_warning(4197)
endif()

if(WIN32)
	add_definitions(-DUNICODE)
	add_definitions(-D_UNICODE)
endif()

add_definitions(-DGALACTIC_OPENSSL=1)
#add_definitions(-DSTATIC_JS_API)

add_definitions(-DCURL_STATICLIB)
add_definitions(-DSDL2_STATIC)

add_definitions(-DENGINE_STATIC)

add_definitions(-DASMJIT_STATIC)

add_subdirectory(Server)
