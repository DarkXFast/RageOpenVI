#include "main.h"

/*
	List of stuff you can replace with this: https://gist.github.com/martonp96/59f731446c7f17db3f400c2be458c4a4

	How to do it:
	Find something from the list, eg. common:/data/dlclist.xml

	Copy the original dlclist.xml from the files and place it in [your game folder]/newmods/common/data/dlclist.xml
	Edit the file, add a new entry like <Item>dlcpacks:/mrpd/</Item>
	Place the single player dlc.rpf like [your game folder]/newmods/dlcpacks/mrpd/dlc.rpf

	Start the game and the modded stuff should be loaded automatically.

	I used this for the example: https://hu.gta5-mods.com/maps/community-mission-row-pd
*/

rage::fiDeviceLocal* platformDevice;
rage::fiDeviceLocal* platformDeviceCRC;
rage::fiDeviceLocal* commonDevice;
rage::fiDeviceLocal* commonDeviceCRC;
rage::fiDeviceLocal* dlcDevice;

bool IsCustomDevice(rage::fiDeviceLocal* dev)
{
	if (dev == platformDevice || dev == platformDeviceCRC || dev == commonDevice || dev == commonDeviceCRC || dev == dlcDevice)
		return true;
	return false;
}

void(*InitialMountOrig)();
void InitialMountHook()
{
	InitialMountOrig();

	std::filesystem::path cwd = std::filesystem::current_path() / "newmods/";
	cwd.make_preferred();

	logger::write("device", "[%s] Mods path: %s", __FUNCTION__, cwd.string().c_str());

	rage::fiDeviceLocal* rootDevice = new rage::fiDeviceLocal();
	rootDevice->SetPath(cwd.string().c_str(), true, nullptr);

	if (rootDevice->Mount("mods:/"))
		logger::write("device", "[%s] Root device mounted!", __FUNCTION__);

	platformDevice = new rage::fiDeviceLocal();
	platformDeviceCRC = new rage::fiDeviceLocal();
	commonDevice = new rage::fiDeviceLocal();
	commonDeviceCRC = new rage::fiDeviceLocal();
	dlcDevice = new rage::fiDeviceLocal();

	platformDevice->SetPath("mods:/platform/", true, nullptr);
	platformDevice->Mount("platform:/");

	platformDeviceCRC->SetPath("mods:/platform/", true, nullptr);
	platformDeviceCRC->Mount("platformcrc:/");

	commonDevice->SetPath("mods:/common/", true, nullptr);
	commonDevice->Mount("common:/");

	commonDeviceCRC->SetPath("mods:/common/", true, nullptr);
	commonDeviceCRC->Mount("commoncrc:/");

	dlcDevice->SetPath("mods:/dlcpacks/", true, nullptr);
	dlcDevice->Mount("dlcpacks:/");
}

using GetDeviceFn = rage::fiDevice* (__fastcall*)(const char*, bool, bool);
static GetDeviceFn GetDevice = nullptr;

char (*OpenArchiveOrig)(rage::fiPackfile* a1, const char* path, char smth, int32_t type, __int64 a5, __int64 a6, bool a7);
char OpenArchiveHook(rage::fiPackfile* a1, const char* path, char smth, int32_t type, __int64 a5, __int64 a6, bool a7)
{
	auto device = GetDevice ? GetDevice(path, true, true) : nullptr;
	if (device && IsCustomDevice((rage::fiDeviceLocal*)device))
	{
		if (device->GetAttributes(path) & FILE_ATTRIBUTE_DIRECTORY)
		{
			logger::write("device", "[%s] Archive %s is a directory!", __FUNCTION__, path);
			type = 2;
		}
	}

	logger::write("device", "[%s] Opening archive %s %d %d", __FUNCTION__, path, smth, type);

	return OpenArchiveOrig(a1, path, smth, type, a5, a6, a7);
}

static memory::InitFuncs CustomDevice([] {

		// The mods devices need Mount/SetPath (rage/fiDevice.cpp); without them, mounting would call address 0.
		bool canMount = patterns::find(patterns::DeviceMount) && patterns::find(patterns::DeviceSetPath);

		if (auto call = patterns::find(patterns::InitialMountCall); call && canMount)
		{
			memory mem(call);
			InitialMountOrig = mem.add(1).rip().as<decltype(InitialMountOrig)>();
			mem.set_call(InitialMountHook);
		}

		GetDevice = reinterpret_cast<GetDeviceFn>(patterns::find(patterns::GetDevice));

		if (auto fn = patterns::find(patterns::OpenArchive))
			memory(fn).hook(OpenArchiveHook, &OpenArchiveOrig);
});