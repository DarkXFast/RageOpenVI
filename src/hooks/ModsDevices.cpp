#include "main.h"
#include <mutex>
#include <unordered_map>
#include <unordered_set>

/*
	Mods folder: `<game folder>/mods/<mount name>/...`

	GTA VI keeps a stack of devices per mount point (`platform:/`, `common:/`, ...). A file lookup starts at the most
	recently mounted device and moves down the stack while a device answers "not found". So a device that is always on
	top and maps the mount point to a folder on disk overrides exactly the files that folder contains, loose files and
	whole .rpf archives alike, and everything else still comes from the game.

	Every time the game mounts a device, this hook looks at the mount name. If `mods/<name>/` exists, a folder device
	for it is created once and (re)mounted on top of that mount point, so it stays above devices the game adds later.

	Mount names come from the game's data, not from the exe, so they're logged ("device" log category) to show which
	folders can be used. Examples: `platform:/` -> `mods/platform/`, `common:/data/` -> `mods/common/data/`.
	A name ending in `crc` (e.g. `commoncrc:/`) falls back to the folder without it (`mods/common/`), as RageOpenRDR2 did.
*/

namespace
{
	using MountFn = bool(__fastcall*)(rage::fiDevice*, const char*);
	MountFn g_mountOrig = nullptr;

	std::recursive_mutex g_lock;
	std::unordered_map<std::string, rage::fiDevice*> g_byName;   // nullptr = no folder for this mount name
	std::unordered_set<rage::fiDevice*> g_ours;

	std::filesystem::path ModsRoot()
	{
		wchar_t exe[MAX_PATH]{ 0 };
		GetModuleFileNameW(nullptr, exe, MAX_PATH);
		return std::filesystem::path(exe).parent_path() / "mods";
	}

	bool SafePart(const std::string& s, bool allowSlash)
	{
		if (s.find("..") != std::string::npos)
			return false;
		for (char c : s)
		{
			bool ok = isalnum((unsigned char)c) || c == '_' || c == '-' || c == '.' || (allowSlash && (c == '/' || c == '\\'));
			if (!ok)
				return false;
		}
		return true;
	}

	// "platform:/levels/" -> <mods>/platform/levels/ if it exists, else empty.
	std::string FolderFor(const std::string& mountName)
	{
		auto colon = mountName.find(':');
		if (colon == std::string::npos || colon == 0)
			return {};

		std::string root = mountName.substr(0, colon);
		std::string rest = mountName.substr(colon + 1);
		while (!rest.empty() && (rest.front() == '/' || rest.front() == '\\'))
			rest.erase(rest.begin());
		if (!SafePart(root, false) || !SafePart(rest, true))
			return {};

		std::vector<std::string> roots{ root };
		if (root.size() > 3 && _stricmp(root.c_str() + root.size() - 3, "crc") == 0)
			roots.push_back(root.substr(0, root.size() - 3));

		std::error_code ec;
		for (auto& r : roots)
		{
			auto dir = ModsRoot() / r / rest;
			if (std::filesystem::is_directory(dir, ec))
			{
				auto s = dir.make_preferred().string();
				if (s.back() != '\\')
					s += '\\';
				return s;
			}
		}
		return {};
	}

	bool MountHook(rage::fiDevice* device, const char* name)
	{
		bool ok = g_mountOrig(device, name);
		if (!ok || !name)
			return ok;

		std::lock_guard guard(g_lock);
		if (g_ours.count(device))
			return ok;

		logger::write("device", "[%s] game mounted %p on %s", __FUNCTION__, device, name);

		rage::fiDevice* ours = nullptr;
		auto it = g_byName.find(name);
		if (it == g_byName.end())
		{
			auto folder = FolderFor(name);
			if (!folder.empty())
			{
				ours = rage::device::create_folder_device(folder);
				if (ours)
				{
					g_ours.insert(ours);
					logger::write("mods", "[%s] %s -> %s", __FUNCTION__, name, folder.c_str());
				}
			}
			g_byName.emplace(name, ours);
		}
		else
		{
			ours = it->second;
		}

		if (ours)
		{
			// Keep the mods device on top of this mount point's stack.
			rage::device::unmount(ours);
			if (!g_mountOrig(ours, name))
				logger::write("mods", "[%s] couldn't mount mods device on %s", __FUNCTION__, name);
		}
		return ok;
	}
}

static memory::InitFuncs ModsDevices([] {
	if (!rage::device::init())
	{
		logger::write("info", " Mods devices disabled: a device function wasn't found");
		return;
	}

	logger::write("info", " Mods folder: %s", ModsRoot().string().c_str());

	memory(rage::device::mount_address()).hook(MountHook, &g_mountOrig);
	if (g_mountOrig)
		rage::device::set_original_mount(reinterpret_cast<void*>(g_mountOrig));
});
