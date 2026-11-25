#include "main.h"

// Logs every archive the game opens by path ("rpf" log category), and whether it opened. Useful on a new build to
// see archive names and to check that an .rpf placed in the mods folder is the one being opened.

namespace
{
	using OpenArchiveFn = void* (__fastcall*)(void**, const char*, const void*);
	OpenArchiveFn g_openArchiveOrig = nullptr;

	void* OpenArchiveHook(void** out, const char* path, const void* header)
	{
		void* ret = g_openArchiveOrig(out, path, header);
		logger::write("rpf", "[%s] %s %s", __FUNCTION__, path ? path : "(null)", (out && *out) ? "opened" : "FAILED");
		return ret;
	}
}

static memory::InitFuncs ArchiveLog([] {
	if (!config::get_log("rpf"))
		return; // no point hooking if nothing is logged

	if (auto fn = patterns::find_any(patterns::OpenArchive, patterns::fallback::OpenArchive))
		memory(fn).hook(OpenArchiveHook, &g_openArchiveOrig);
});
