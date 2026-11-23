#include "main.h"

namespace
{
	using MountFn = bool(__fastcall*)(rage::fiDevice*, const char*);
	using UnmountFn = void(__fastcall*)(rage::fiDevice*);
	using SetPathFn = void(__fastcall*)(rage::fiDevice*, const char*, bool, rage::fiDevice*);
	using CtorFn = rage::fiDevice* (__fastcall*)(void*);
	using ParentFn = rage::fiDevice* (__fastcall*)(bool);

	MountFn g_mount = nullptr;      // the game's Mount; replaced by the trampoline once it's hooked
	UnmountFn g_unmount = nullptr;
	SetPathFn g_setPath = nullptr;
	CtorFn g_ctor = nullptr;
	ParentFn g_parent = nullptr;
	uintptr_t g_mountAddress = 0;

	// The game allocates 0x118 bytes for a relative device on Xbox. Allocate more in case the PC object is bigger;
	// mount devices live for the whole session, so this is never freed.
	constexpr size_t kDeviceAlloc = 0x400;
}

bool rage::device::init()
{
	g_mountAddress = patterns::find(patterns::DeviceMount);
	g_mount = reinterpret_cast<MountFn>(g_mountAddress);
	g_unmount = reinterpret_cast<UnmountFn>(patterns::find(patterns::DeviceUnmount));
	g_setPath = reinterpret_cast<SetPathFn>(patterns::find(patterns::DeviceSetPath));
	g_ctor = reinterpret_cast<CtorFn>(patterns::find(patterns::RelativeDeviceCtor));
	g_parent = reinterpret_cast<ParentFn>(patterns::find(patterns::ParentDevice));
	return g_mount && g_unmount && g_setPath && g_ctor && g_parent;
}

rage::fiDevice* rage::device::create_folder_device(const std::string& folder)
{
	if (!g_ctor || !g_setPath || !g_parent)
		return nullptr;

	fiDevice* local = g_parent(false);
	if (!local)
		return nullptr;

	void* mem = _aligned_malloc(kDeviceAlloc, 16);
	if (!mem)
		return nullptr;
	memset(mem, 0, kDeviceAlloc);

	fiDevice* dev = g_ctor(mem);
	g_setPath(dev, folder.c_str(), true, local);
	logger::write("device", "[%s] %s -> device %p (parent Local %p)", __FUNCTION__, folder.c_str(), dev, local);
	return dev;
}

bool rage::device::mount(fiDevice* device, const char* mountName)
{
	return g_mount ? g_mount(device, mountName) : false;
}

void rage::device::unmount(fiDevice* device)
{
	if (g_unmount)
		g_unmount(device);
}

uintptr_t rage::device::mount_address()
{
	return g_mountAddress;
}

void rage::device::set_original_mount(void* original)
{
	g_mount = reinterpret_cast<MountFn>(original);
}
