
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
	mozjs-52
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
	mozjs-60
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
	add_custom_command(TARGET Server POST_BUILD COMMAND copy \"${SPIDERMONKEY_LIBS}\\$<$<CONFIG:Debug>:Debug>$<$<NOT:$<CONFIG:Debug>>:Release>\\*.dll\" \"$<TARGET_FILE_DIR:Server>\" WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

	install(DIRECTORY "${SPIDERMONKEY_LIBS}/Debug/" DESTINATION "${LIB_DIRECTORY}/Debug" CONFIGURATIONS Debug FILES_MATCHING PATTERN "*.dll")
	install(DIRECTORY "${SPIDERMONKEY_LIBS}/Release/" DESTINATION "${LIB_DIRECTORY}/Release" CONFIGURATIONS Release FILES_MATCHING PATTERN "*.dll")
	install(DIRECTORY "${SPIDERMONKEY_LIBS}/Release/" DESTINATION "${LIB_DIRECTORY}/RelWithDebInfo" CONFIGURATIONS RelWithDebInfo FILES_MATCHING PATTERN "*.dll")

	#copy_dependency("${DEPENDENCIES_LIBS}" SDL2 dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" enet dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" curl dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" crypto-42 dll)
	#copy_dependency("${DEPENDENCIES_LIBS}" ssl-44 dll)
endif()
