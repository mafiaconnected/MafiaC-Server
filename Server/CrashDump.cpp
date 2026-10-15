#include "pch.h"
#include "CrashDump.h"
#include "ServerVersion.h"

#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#include <signal.h>
#include <stdlib.h>
#else
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>
#endif

#ifdef _WIN32

// dbghelp is loaded at runtime rather than linked so a missing or old copy never stops the server from starting.
typedef BOOL(WINAPI* MiniDumpWriteDumpProc)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

static MiniDumpWriteDumpProc g_pMiniDumpWriteDump = nullptr;
static wchar_t g_szCrashDir[MAX_PATH];
static volatile LONG g_lCrashing = 0;

static void WriteMiniDump(EXCEPTION_POINTERS* pExceptionInfo)
{
	if (InterlockedExchange(&g_lCrashing, 1) != 0)
		return;

	if (g_pMiniDumpWriteDump == nullptr)
	{
		fwprintf(stderr, L"*** Server crashed, but dbghelp.dll is unavailable so no dump was written ***\n");
		return;
	}

	CreateDirectoryW(g_szCrashDir, nullptr);

	SYSTEMTIME Time;
	GetLocalTime(&Time);

	wchar_t szPath[MAX_PATH + 64];
	_snwprintf_s(szPath, _TRUNCATE, L"%s\\crash_%04u%02u%02u_%02u%02u%02u_%lu.dmp", g_szCrashDir,
		Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond, GetCurrentProcessId());

	HANDLE hFile = CreateFileW(szPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		fwprintf(stderr, L"*** Server crashed, but %s could not be created ***\n", szPath);
		return;
	}

	MINIDUMP_EXCEPTION_INFORMATION ExceptionInfo;
	ExceptionInfo.ThreadId = GetCurrentThreadId();
	ExceptionInfo.ExceptionPointers = pExceptionInfo;
	ExceptionInfo.ClientPointers = FALSE;

	// All of these exist in the dbghelp that ships with XP
	MINIDUMP_TYPE Type = (MINIDUMP_TYPE)(MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithHandleData);
	BOOL bWritten = g_pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, Type,
		pExceptionInfo != nullptr ? &ExceptionInfo : nullptr, nullptr, nullptr);
	CloseHandle(hFile);

	if (bWritten)
		fwprintf(stderr, L"*** Server crashed, dump written to %s ***\n", szPath);
	else
	{
		fwprintf(stderr, L"*** Server crashed, but writing %s failed (error %lu) ***\n", szPath, GetLastError());
		DeleteFileW(szPath);
	}
}

static LONG WINAPI CrashExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
	WriteMiniDump(pExceptionInfo);
	return EXCEPTION_CONTINUE_SEARCH;
}

// abort(), pure virtual calls and CRT invalid parameters terminate without ever reaching the unhandled exception
// filter, so raise an exception just to get a context to put in the dump.
static void DumpFromHere()
{
	__try
	{
		RaiseException(0xE0000001, 0, 0, nullptr);
	}
	__except (WriteMiniDump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

static void __cdecl AbortHandler(int iSignal)
{
	DumpFromHere();
}

static void __cdecl PureCallHandler()
{
	DumpFromHere();
	TerminateProcess(GetCurrentProcess(), 3);
}

static void __cdecl InvalidParameterHandler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
	DumpFromHere();
	TerminateProcess(GetCurrentProcess(), 3);
}

void CCrashDump::Install()
{
	DWORD dwLength = GetCurrentDirectoryW(ARRAY_COUNT(g_szCrashDir) - 9, g_szCrashDir);
	if (dwLength == 0 || dwLength >= ARRAY_COUNT(g_szCrashDir) - 9)
		g_szCrashDir[0] = L'.', g_szCrashDir[1] = L'\0';
	wcscat_s(g_szCrashDir, L"\\crashes");

	// Loaded now, not inside the filter, since taking the loader lock in a crashed process can deadlock
	HMODULE hDbgHelp = LoadLibraryW(L"dbghelp.dll");
	if (hDbgHelp != nullptr)
		g_pMiniDumpWriteDump = (MiniDumpWriteDumpProc)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

	SetUnhandledExceptionFilter(CrashExceptionFilter);

	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
	signal(SIGABRT, AbortHandler);
	_set_purecall_handler(PureCallHandler);
	_set_invalid_parameter_handler(InvalidParameterHandler);
}

#else

// Everything the signal handler uses is prepared up front, since it may only make async-signal-safe calls.
static char g_szCrashDir[PATH_MAX];
static char g_szGCore[PATH_MAX];
static volatile sig_atomic_t g_iCrashing = 0;

static const int g_Signals[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT, SIGSYS };

static void WriteString(int fd, const char* psz)
{
	size_t Length = strlen(psz);
	while (Length > 0)
	{
		ssize_t Written = write(fd, psz, Length);
		if (Written <= 0)
			return;
		psz += Written;
		Length -= Written;
	}
}

static char* FormatUnsigned(char* pszBuffer, size_t Size, uint64_t uiValue, unsigned int uiBase)
{
	char* psz = pszBuffer + Size - 1;
	*psz = '\0';
	do
	{
		*--psz = "0123456789abcdef"[uiValue % uiBase];
		uiValue /= uiBase;
	} while (uiValue != 0 && psz > pszBuffer);
	return psz;
}

static void WriteDecimal(int fd, int64_t iValue)
{
	char szBuffer[24];
	if (iValue < 0)
	{
		WriteString(fd, "-");
		iValue = -iValue;
	}
	WriteString(fd, FormatUnsigned(szBuffer, sizeof(szBuffer), (uint64_t)iValue, 10));
}

static void WriteHex(int fd, uint64_t uiValue)
{
	char szBuffer[24];
	WriteString(fd, "0x");
	WriteString(fd, FormatUnsigned(szBuffer, sizeof(szBuffer), uiValue, 16));
}

static void AppendString(char* pszBuffer, size_t Size, const char* psz)
{
	size_t Length = strlen(pszBuffer);
	while (*psz != '\0' && Length + 1 < Size)
		pszBuffer[Length++] = *psz++;
	pszBuffer[Length] = '\0';
}

static void AppendDecimal(char* pszBuffer, size_t Size, uint64_t uiValue)
{
	char szNumber[24];
	AppendString(pszBuffer, Size, FormatUnsigned(szNumber, sizeof(szNumber), uiValue, 10));
}

static const char* GetSignalName(int iSignal)
{
	switch (iSignal)
	{
		case SIGSEGV: return "SIGSEGV";
		case SIGBUS: return "SIGBUS";
		case SIGFPE: return "SIGFPE";
		case SIGILL: return "SIGILL";
		case SIGABRT: return "SIGABRT";
		case SIGSYS: return "SIGSYS";
		default: return "?";
	}
}

static void CopyFile(int fdOut, const char* pszPath)
{
	int fdIn = open(pszPath, O_RDONLY);
	if (fdIn < 0)
		return;
	char Buffer[4096];
	ssize_t Read;
	while ((Read = read(fdIn, Buffer, sizeof(Buffer))) > 0)
	{
		if (write(fdOut, Buffer, Read) != Read)
			break;
	}
	close(fdIn);
}

static void WriteCrashLog(const char* pszPath, int iSignal, siginfo_t* pInfo, void* pContext)
{
	int fd = open(pszPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0)
		return;

	WriteString(fd, "MafiaC Server " MAFIAC_SERVER_VERSION " crash\n\nSignal: ");
	WriteDecimal(fd, iSignal);
	WriteString(fd, " (");
	WriteString(fd, GetSignalName(iSignal));
	WriteString(fd, ")\nCode: ");
	WriteDecimal(fd, pInfo->si_code);
	WriteString(fd, "\nFault address: ");
	WriteHex(fd, (uint64_t)(uintptr_t)pInfo->si_addr);

	ucontext_t* pUContext = (ucontext_t*)pContext;
#if defined(__x86_64__)
	WriteString(fd, "\nInstruction pointer: ");
	WriteHex(fd, (uint64_t)pUContext->uc_mcontext.gregs[REG_RIP]);
#elif defined(__i386__)
	WriteString(fd, "\nInstruction pointer: ");
	WriteHex(fd, (uint64_t)pUContext->uc_mcontext.gregs[REG_EIP]);
#elif defined(__aarch64__)
	WriteString(fd, "\nInstruction pointer: ");
	WriteHex(fd, (uint64_t)pUContext->uc_mcontext.pc);
#else
	(void)pUContext;
#endif

	WriteString(fd, "\nPID: ");
	WriteDecimal(fd, getpid());
	WriteString(fd, "\nTime (unix): ");
	WriteDecimal(fd, time(nullptr));

	WriteString(fd, "\n\nBacktrace:\n");
	void* Frames[128];
	int iFrames = backtrace(Frames, ARRAY_COUNT(Frames));
	backtrace_symbols_fd(Frames, iFrames, fd);

	// Lets the backtrace addresses be resolved with addr2line against the module's load address
	WriteString(fd, "\nMemory map:\n");
	CopyFile(fd, "/proc/self/maps");

	close(fd);
}

// Runs gcore on this process and waits for it. Returns true when it produced a core.
static bool WriteCore(const char* pszPrefix)
{
	int Pipe[2];
	if (pipe(Pipe) != 0)
		return false;

	char szPid[24];
	char* pszPid = FormatUnsigned(szPid, sizeof(szPid), (uint64_t)getpid(), 10);

	pid_t Child = fork();
	if (Child < 0)
	{
		close(Pipe[0]);
		close(Pipe[1]);
		return false;
	}

	if (Child == 0)
	{
		// Wait until the parent has allowed us to attach to it
		close(Pipe[1]);
		char c;
		while (read(Pipe[0], &c, 1) < 0 && errno == EINTR) {}
		close(Pipe[0]);

		int fdNull = open("/dev/null", O_WRONLY);
		if (fdNull >= 0)
		{
			dup2(fdNull, STDOUT_FILENO);
			dup2(fdNull, STDERR_FILENO);
		}
		execl(g_szGCore, "gcore", "-o", pszPrefix, pszPid, (char*)nullptr);
		_exit(127);
	}

	// With Yama ptrace_scope 1 only ancestors may attach, so name gcore (and the gdb it starts) as an allowed tracer.
	// Fails harmlessly with EINVAL when Yama isn't present.
	prctl(PR_SET_PTRACER, (unsigned long)Child, 0, 0, 0);
	close(Pipe[0]);
	close(Pipe[1]);

	// Give up after 5 minutes rather than leave a hung server behind
	struct timespec Delay = { 0, 100 * 1000 * 1000 };
	for (int i = 0; i < 3000; i++)
	{
		int iStatus;
		pid_t Result = waitpid(Child, &iStatus, WNOHANG);
		if (Result == Child)
			return WIFEXITED(iStatus) && WEXITSTATUS(iStatus) == 0;
		if (Result < 0 && errno != EINTR)
			return false;
		nanosleep(&Delay, nullptr);
	}
	kill(Child, SIGKILL);
	waitpid(Child, nullptr, 0);
	return false;
}

static void CrashSignalHandler(int iSignal, siginfo_t* pInfo, void* pContext)
{
	if (__sync_lock_test_and_set(&g_iCrashing, 1) != 0)
	{
		// Another thread is already writing the dump and will take the process down when it's done
		for (;;)
			pause();
	}

	int iSavedErrno = errno;

	mkdir(g_szCrashDir, 0755);

	char szName[PATH_MAX];
	szName[0] = '\0';
	AppendString(szName, sizeof(szName), g_szCrashDir);
	AppendString(szName, sizeof(szName), "/crash_");
	AppendDecimal(szName, sizeof(szName), (uint64_t)time(nullptr));
	AppendString(szName, sizeof(szName), "_");
	AppendDecimal(szName, sizeof(szName), (uint64_t)getpid());

	char szLog[PATH_MAX];
	szLog[0] = '\0';
	AppendString(szLog, sizeof(szLog), szName);
	AppendString(szLog, sizeof(szLog), ".log");

	WriteString(STDERR_FILENO, "\n*** Server crashed with ");
	WriteString(STDERR_FILENO, GetSignalName(iSignal));
	WriteString(STDERR_FILENO, ", writing ");
	WriteString(STDERR_FILENO, szLog);
	WriteString(STDERR_FILENO, " ***\n");

	WriteCrashLog(szLog, iSignal, pInfo, pContext);

	if (g_szGCore[0] != '\0')
	{
		// gcore appends .<pid> to the prefix
		WriteString(STDERR_FILENO, "*** Writing core with gcore, this can take a while ... ***\n");
		if (WriteCore(szName))
		{
			WriteString(STDERR_FILENO, "*** Core written to ");
			WriteString(STDERR_FILENO, szName);
			WriteString(STDERR_FILENO, ".");
			WriteDecimal(STDERR_FILENO, getpid());
			WriteString(STDERR_FILENO, " ***\n");

			// Already have a core, don't make the kernel write another
			struct rlimit NoCore = { 0, 0 };
			setrlimit(RLIMIT_CORE, &NoCore);
		}
		else
			WriteString(STDERR_FILENO, "*** gcore failed, leaving the core to the kernel ***\n");
	}

	// Die from the original signal, so the exit status and any kernel core dump are what they'd have been without us
	struct sigaction Action;
	memset(&Action, 0, sizeof(Action));
	Action.sa_handler = SIG_DFL;
	sigemptyset(&Action.sa_mask);
	sigaction(iSignal, &Action, nullptr);

	errno = iSavedErrno;
	raise(iSignal); // Delivered once the handler returns, since iSignal is blocked while it runs
}

static void FindGCore()
{
	static const char* s_Paths[] = { "/usr/bin/gcore", "/usr/local/bin/gcore", "/bin/gcore" };
	g_szGCore[0] = '\0';
	for (size_t i = 0; i < ARRAY_COUNT(s_Paths); i++)
	{
		if (access(s_Paths[i], X_OK) == 0)
		{
			AppendString(g_szGCore, sizeof(g_szGCore), s_Paths[i]);
			return;
		}
	}
}

void CCrashDump::Install()
{
	if (getcwd(g_szCrashDir, sizeof(g_szCrashDir) - 9) == nullptr)
		strcpy(g_szCrashDir, ".");
	AppendString(g_szCrashDir, sizeof(g_szCrashDir), "/crashes");

	FindGCore();

	// The first backtrace() call loads libgcc, which allocates, so get that out of the way now
	void* Frames[1];
	backtrace(Frames, 1);

	// Fallback for when gcore isn't installed or fails: let the kernel write a core (wherever core_pattern says)
	struct rlimit Limit;
	if (getrlimit(RLIMIT_CORE, &Limit) == 0 && Limit.rlim_cur != Limit.rlim_max)
	{
		Limit.rlim_cur = Limit.rlim_max;
		setrlimit(RLIMIT_CORE, &Limit);
	}
	prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);

	// A stack overflow leaves no stack to run the handler on
	static char s_AltStack[64 * 1024];
	stack_t Stack;
	memset(&Stack, 0, sizeof(Stack));
	Stack.ss_sp = s_AltStack;
	Stack.ss_size = sizeof(s_AltStack);
	sigaltstack(&Stack, nullptr);

	struct sigaction Action;
	memset(&Action, 0, sizeof(Action));
	Action.sa_sigaction = CrashSignalHandler;
	Action.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigemptyset(&Action.sa_mask);
	for (size_t i = 0; i < ARRAY_COUNT(g_Signals); i++)
		sigaction(g_Signals[i], &Action, nullptr);
}

#endif
