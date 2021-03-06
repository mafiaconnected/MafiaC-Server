// Main.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include "Server.h"
#include <openssl/ssl.h>
#ifdef _WIN32
#include <crtdbg.h>
#include <conio.h>
#else
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#endif
#include <Engine/SignalHandlers.h>
#ifdef _WIN32
#include "resource.h"
#else
#include <MafiaCServerArchive.h>
#endif

static CServer* g_pServer = NULL;

#ifdef _WIN32
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#ifndef _WIN32
char getch()
{
	char buf = 0;
	struct termios old = { 0 };
	fflush(stdout);
	if (tcgetattr(0, &old) < 0)
		perror("tcsetattr()");
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &old) < 0)
		perror("tcsetattr ICANON");
	if (read(0, &buf, 1) < 0)
		perror("read()");
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	if (tcsetattr(0, TCSADRAIN, &old) < 0)
		perror("tcsetattr ~ICANON");
	//printf("%c\n", buf);
	return buf;
}
#endif

static bool LoadTARArchive(CFileSystem* pFileSystem, Stream* pStream, const GChar* pszTarget, bool bCaseSensitive, bool bExclusive)
{
	CTARArchive* pArchive = new CTARArchive;
	pArchive->SetCaseSensitive(bCaseSensitive);
	if (pArchive->Read(pStream))
	{
		pFileSystem->Mount(pArchive, pszTarget, bExclusive);
		return true;
	}
	else
		delete pArchive;
	return false;
}

#if !defined(_WIN32) && !defined(G_T_A_C_SERVER_ARCHIVE_H)
static bool LoadTARArchive(CFileSystem* pFileSystem, const GChar* pszArchive, const GChar* pszTarget, bool bCaseSensitive, bool bExclusive)
{
	auto pStream = Strong<Stream>::New(pFileSystem->Open(pszArchive, false));
	if (pStream != nullptr)
	{
		return LoadTARArchive(pFileSystem, pStream, pszTarget, bCaseSensitive, bExclusive);
	}
	return false;
}
#endif

template<class T> class Optional
{
public:
	Optional() : m_bData(false) {}

	bool m_bData;
	T m_Data;

	operator T() { return m_Data; }
};

#ifdef WIN32
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
#ifdef _WIN32
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_WNDW);
	_set_error_mode(_OUT_TO_MSGBOX);
#endif

	curl_global_init(CURL_GLOBAL_ALL);
	enet_initialize();
	atexit(curl_global_cleanup);
	atexit(enet_deinitialize);
#ifdef _WIN32 // HACK
	atexit(CRYPTO_cleanup_all_ex_data);
#endif

	try
	{
		Context Context;

		Context.AddUserAgent(_gstr("MafiaCServer/") __gstr(MAFIAC_SERVER_VERSION));

#ifdef _WIN32
		{
			auto pStream = Strong<Stream>::New(OpenResource(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_MAFIACSERVER), _T("TAR")));
			if (pStream != nullptr)
			{
				LoadTARArchive(Context.GetFileSystem(), pStream, _gstr("/"), true, false);
			}
		}
#elif defined(M_A_F_I_A_C_SERVER_ARCHIVE_H)
		{
			auto pStream = Strong<Stream>::New(new CMemoryStream((void*)M_A_F_I_A_C_SERVER, ARRAY_COUNT(M_A_F_I_A_C_SERVER), true, false));
			if (pStream != nullptr)
			{
				LoadTARArchive(Context.GetFileSystem(), pStream, _gstr("/"), true, false);
			}
		}
#else
		LoadTARArchive(Context.GetFileSystem(), _gstr("/MafiaCServer.tar"), _gstr("/"), true, false);
#endif

		CServer Server(&Context);
		g_pServer = &Server;

		CSignalHandlers::Install([]() {
			g_pServer->m_ExitSignal.Signal();
		});

		bool bDumpDoc = false;
		bool bExpectConfig = false;
		bool bExpectNumber = false;
		bool* pbFlag;
		int32_t* piNumber;
		bool bNoInput = false;
		GString Config = _gstr("server.xml");
		Optional<int32_t> iPort;
		Optional<int32_t> iHttpPort;
		Optional<int32_t> iRconPort;
		Optional<int32_t> iMaxPlayers;

		for (int i = 0; i < argc; i++)
		{
			CString Arg(false, argv[i]);
			if (bExpectConfig)
			{
				Config.assign(Arg.CString(), Arg.GetLength());
				bExpectConfig = false;
			}
			else if (bExpectNumber)
			{
				*piNumber = _gatoi(Arg);
				*pbFlag = true;
				bExpectNumber = false;
			}
			else if (_gstrcasecmp(Arg, _gstr("-dumpdoc")) == 0)
				bDumpDoc = true;
			else if (_gstrcasecmp(Arg, _gstr("-config")) == 0)
				bExpectConfig = true;
			else if (_gstrcasecmp(Arg, _gstr("-noinput")) == 0)
				bNoInput = true;
			else if (_gstrcasecmp(Arg, _gstr("-port")) == 0)
			{
				pbFlag = &iPort.m_bData;
				piNumber = &iPort.m_Data;
				bExpectNumber = true;
			}
			else if (_gstrcasecmp(Arg, _gstr("-httpport")) == 0)
			{
				pbFlag = &iHttpPort.m_bData;
				piNumber = &iHttpPort.m_Data;
				bExpectNumber = true;
			}
			else if (_gstrcasecmp(Arg, _gstr("-rconport")) == 0)
			{
				pbFlag = &iRconPort.m_bData;
				piNumber = &iRconPort.m_Data;
				bExpectNumber = true;
			}
			else if (_gstrcasecmp(Arg, _gstr("-maxplayers")) == 0)
			{
				pbFlag = &iMaxPlayers.m_bData;
				piNumber = &iMaxPlayers.m_Data;
				bExpectNumber = true;
			}
		}

		bool bSuccess = false;

		if (bSuccess = Server.LoadConfig(Config.c_str()))
		{
			if (iPort.m_bData)
			{
				Server.m_usPort = (uint16_t)iPort;
			}

			if (iHttpPort.m_bData)
			{
				Server.m_usHTTPPort = (uint16_t)iHttpPort;
			}

			if (iRconPort.m_bData)
			{
				Server.m_usRConPort = (uint16_t)iRconPort;
			}
			if (iMaxPlayers.m_bData)
			{
				Server.m_MaxClients = (size_t)iMaxPlayers;
			}

			if (bSuccess = Server.StartServer())
			{
				{
					auto pStream = Strong<Stream>::New(Context.GetFileSystem()->Open(_gstr("/commandline.txt"), false));
					if (pStream != nullptr)
					{
						Server.m_ResourceMgr.m_pCommandHandlers->DoCommandLine(pStream, true);
					}
				}

				if (bDumpDoc)
				{
					Server.m_ResourceMgr.m_pCommandHandlers->ConsoleCommand(_gstr("/dumpdoc"), nullptr, true);
				}

				if (!bNoInput)
					Server.StartInputThread();

				Server.MainLoop();
			}
		}

		if (!bSuccess)
		{
			_gprintf(_gstr("Press any key to continue ..."));
			getch();
			_gprintf(_gstr("\n"));
			return EXIT_FAILURE;
		}
		else
			Server.ShutDown();
	}
	catch (std::exception& e)
	{
		Context::ShowExceptionMessage(e, false);
	}

	return EXIT_SUCCESS;
}
