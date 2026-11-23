#include "main.h"

bool(*ParseHeaderOrig)(rage::fiPackfile*, const char*, bool, void*, rage::fiResourceInfo*, bool);
bool ParseHeaderHook(rage::fiPackfile* a1, const char* name, bool readHeader, void* customHeader, rage::fiResourceInfo* a5, bool a6)
{
	logger::log("mods", "[%s] parsing header /%s", __FUNCTION__, name);
	bool ret = ParseHeaderOrig(a1, name, readHeader, customHeader, a5, a6);
	return ret;
}

static memory::InitFuncs PackfileEncryptionHooks([] {
	if (auto call = patterns::find(patterns::ParseHeaderCall))
	{
		memory mem(call);
		ParseHeaderOrig = mem.add(1).rip().as<decltype(ParseHeaderOrig)>();
		mem.set_call(ParseHeaderHook);
	}
});