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

// The size the game itself allocates for a relative device, read from its `mov ecx, size` before `new`.
// Returns false if it's bigger than kDeviceAlloc, so the caller turns mods devices off instead of overrunning.
static bool CheckDeviceSize()
{
	uintptr_t at = patterns::find_any(patterns::RelativeDeviceSize, patterns::fallback::RelativeDeviceSize);
	if (!at)
	{
		logger::write("info", " Relative device size not found; assuming it fits in 0x%zx bytes", kDeviceAlloc);
		return true;
	}
	uint32_t size = *reinterpret_cast<const uint32_t*>(at);
	logger::write("info", " Relative device size: 0x%x (allocating 0x%zx)", size, kDeviceAlloc);
	if (size > kDeviceAlloc)
	{
		logger::write("info", " !! The game's relative device is bigger than 0x%zx bytes; raise kDeviceAlloc", kDeviceAlloc);
		return false;
	}
	return true;
}

bool rage::device::init()
{
	using namespace patterns;
	g_mountAddress = find_any(DeviceMount, fallback::DeviceMount);
	g_mount = reinterpret_cast<MountFn>(g_mountAddress);
	g_unmount = reinterpret_cast<UnmountFn>(find_any(DeviceUnmount, fallback::DeviceUnmount));
	g_setPath = reinterpret_cast<SetPathFn>(find_any(DeviceSetPath, fallback::DeviceSetPath));
	g_ctor = reinterpret_cast<CtorFn>(find_any(RelativeDeviceCtor, fallback::RelativeDeviceCtor));
	g_parent = reinterpret_cast<ParentFn>(find_any(ParentDevice, fallback::ParentDevice));
	bool sizeOk = CheckDeviceSize();
	return g_mount && g_unmount && g_setPath && g_ctor && g_parent && sizeOk;
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
