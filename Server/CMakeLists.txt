
if(UNIX)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")
endif()

list(APPEND CMAKE_PROGRAM_PATH "${CMAKE_SOURCE_DIR}/Tools/" ...)
find_program(BIN2H bin2h)
if(NOT BIN2H)
	message(FATAL_ERROR "bin2h not found!")
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
execute_process(COMMAND ${BIN2H} "${CMAKE_SOURCE_DIR}/MafiaCServer.tar" -o "${CMAKE_BINARY_DIR}/generated/MafiaServerArchive.h")

include_directories(${CMAKE_BINARY_DIR}/generated)

file(GLOB_RECURSE SOURCES *.cpp *.h *.rc)

group_sources("${SOURCES}" "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

if(MSVC)
	precompiled_header(pch)
endif()

new_library_executable(Server "${SOURCES}" "")

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
	libSDL2.a libssl.so libcrypto.so libz.a libpng16.a libtinyxml.a dl pthread libcurl.a
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
	#add_custom_command(TARGET Server POST_BUILD COMMAND copy \"${SPIDERMONKEY_LIBS}\\$<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>\\*.dll\" \"$<TARGET_FILE_DIR:Server>\" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

	install_spidermonkey_to("")

	#copy_dependency("${DEPENDENCIES_LIBS}" SDL2 dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" enet dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" curl dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" crypto-42 dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" ssl-44 dll)
endif()
