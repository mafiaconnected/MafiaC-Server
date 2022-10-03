@echo off
set CMAKE_GENERATOR="Visual Studio 15 2017"
set CMAKE_GENERATOR_PLATFORM=x64
set CMAKE_GENERATOR_TOOLSET=v141_xp
set CMAKE_EXTRA_ARGS=
call Compile.bat
if not defined AUTOMATION pause