#pragma once

// Writes a dump into ./crashes when the server crashes.
// Windows: a minidump (.dmp) via dbghelp.
// Linux: a crash log (.log) with the signal, backtrace and memory map, plus a full core file via gcore when it's installed.
class CCrashDump
{
public:
	static void Install();
};
