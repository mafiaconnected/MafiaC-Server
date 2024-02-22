
if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()

list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_SOURCE_DIR}/Tools/" ...)
find_program(BIN2H bin2h)
if(NOT BIN2H)
	message(FATAL_ERROR "bin2h not found!")
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
execute_process(COMMAND ${BIN2H} "${CMAKE_SOURCE_DIR}/MafiaCServer.tar" -o "${CMAKE_BINARY_DIR}/generated/MafiaCServerArchive.h")

include_directories(${CMAKE_BINARY_DIR}/generated)

file(GLOB_RECURSE SOURCES *.cpp *.h *.rc)

group_sources("${SOURCES}" "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
	precompiled_header(pch)
endif()

new_library_executable(Server "${SOURCES}" "")

if(WIN32)
	set_target_properties(Server PROPERTIES LINK_FLAGS "/MANIFEST:NO")
endif()

if(MSVC AND NOT SUPPORT_XP)
	# Require Windows 7
	target_link_options(Server PRIVATE "/SUBSYSTEM:CONSOLE,6.1")
endif()

target_link_libraries(Server
	PRIVATE
	Multiplayer_static
	Network_static
	Scripting_static
	JSScripting_static
	SquirrelScripting_static
	LuaScripting_static
	FileIntegrity_static
	Galactic_static)

target_link_spidermonkey(Server)

if(WIN32)
target_link_libraries(Server
	PRIVATE
	sqstdlib_static
	squirrel_static
	#RakNet_static
	enet_static
	mongoose6_static
	tinyxml_static
	squirrel_static
	sqstdlib_static
	lua53_static
	crypto_static
	ssl_static
	curl_static
	SDL2
	png_static
	zlib_static
	winmm
	imm32
	version
	shlwapi
	psapi)
else()
target_link_libraries(Server
	PRIVATE
	#js_static
	sqstdlib_static
	squirrel_static
	#RakNet_static
	enet_static
	mongoose6_static
	libssl.a libcrypto.a zlib_static png_static tinyxml_static dl pthread libcurl.a
	SDL2
	squirrel_static
	sqstdlib_static
	lua53_static)
endif()

if(WIN32)
target_link_libraries(Server
	PRIVATE
	wldap32
	crypt32
	ws2_32
	comctl32
	uxtheme
	winmm
	imm32
	version
	advapi32
	setupapi
	shell32
	shlwapi
	psapi
	iphlpapi
	legacy_stdio_definitions)
endif()

if(MSVC)
	install_spidermonkey_to("")

	#copy_dependency("${DEPENDENCIES_LIBS}" SDL2 dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" enet dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" curl dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" crypto-42 dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" ssl-44 dll)
endif()
