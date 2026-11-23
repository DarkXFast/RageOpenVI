#pragma once

// The game's own file devices, used through the game's functions only.
//
// RageOpenRDR2 shipped its own C++ copy of the fiDevice class. That only works if every virtual method sits in the
// same vtable slot as in the game, and GTA VI's device vtable is laid out differently (more than 80 slots,
// GetAttributes at 45 instead of 43, Seek at 15 instead of 16). So this port has no device class of its own: it makes
// mods devices the way the game makes its mount devices (a relative device whose parent is the game's Local device)
// and never calls a virtual method directly.

namespace rage
{
	struct fiDevice; // opaque: only ever passed back to the game

	namespace device
	{
		// Resolves the game functions. Returns false (and logs which ones are missing) if any is missing.
		bool init();

		// A relative device that maps `<mount point>/x` to `<folder>/x` through the game's Local device.
		// Returns nullptr when the functions it needs weren't found.
		fiDevice* create_folder_device(const std::string& folder);

		bool mount(fiDevice* device, const char* mountName);   // the original Mount (not our hook)
		void unmount(fiDevice* device);

		// Mount's address, so the mount hook can be installed on it (0 if not found).
		uintptr_t mount_address();
		void set_original_mount(void* original);
	}
}
