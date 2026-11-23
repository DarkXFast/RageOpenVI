#include "main.h"

static bool bInited = false;
void (*origGetSystemTimeAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);
void HookGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	if (!bInited)
	{
		bInited = true;

		// RageOpenRDR2 also patched RDR2 here (keep the console visible, skip a cache check). Those were RDR2-specific
		// and have no GTA VI equivalent yet; re-check once a PC exe exists.

		memory::InitFuncs::run();

		logger::write("info", " RageOpenVI inited");
	}
	origGetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

static void LogHostProcess()
{
	char path[MAX_PATH]{ 0 };
	GetModuleFileNameA(nullptr, path, MAX_PATH);
	logger::write("info", " Loaded into %s", path);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);

		config::load();

		logger::init();

		if (!config::get_config_or<bool>("enabled", true))
			return TRUE;

		memory::init();

		if (config::get_config<bool>("console"))
		{
			AllocConsole();

			FILE* unused = nullptr;
			freopen_s(&unused, "CONIN$", "r", stdin);
			freopen_s(&unused, "CONOUT$", "w", stdout);
			freopen_s(&unused, "CONOUT$", "w", stderr);
		}

		LogHostProcess();

		if (!memory::HookApi(L"kernel32.dll", "GetSystemTimeAsFileTime", (PVOID)HookGetSystemTimeAsFileTime, (PVOID*)&origGetSystemTimeAsFileTime)) {
			logger::write("info", "Hooking failed error (%ld)", GetLastError());
		}
	}
	return TRUE;
}
