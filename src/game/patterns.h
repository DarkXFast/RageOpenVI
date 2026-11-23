#pragma once

// Every game-specific byte pattern the loader needs, in one place.
//
// STATUS: these were found in the GTA VI **Xbox Series** executable (RVAs below are from that build). GTA VI isn't
// out on PC yet. The PC build should share the same engine code, but its bytes may differ, so each pattern has to be
// checked against the PC exe. A pattern that doesn't match is logged by name and its feature turns itself off.
//
// `offset` is added to the match before use. `kind` says how the result is used.

namespace patterns
{
	enum class Kind
	{
		Function,     // address of a function to call or hook
		CallSite,     // address of an E8 call instruction whose target is replaced
		RipOperand,   // address of a rel32 operand (call target or rip-relative data); the target is read from it
		Patch,        // bytes to patch in place
	};

	struct Pattern
	{
		const char* name;
		const char* signature;
		int offset;
		Kind kind;
	};

	// ---- Used by the loader --------------------------------------------------------------------------------------

	// bool Mount(fiDevice* device, const char* mountName). Pushes `device` on top of the mount point's device stack.
	// Xbox RVA 0x1fb2210.
	inline constexpr Pattern DeviceMount = { "fiDevice::Mount",
		"56 57 53 48 83 ec 20 44 8a 81", 0, Kind::Function };

	// void Unmount(fiDevice* device). Removes `device` from every mount point. Xbox RVA 0x28cb640.
	inline constexpr Pattern DeviceUnmount = { "fiDevice::Unmount",
		"41 57 41 56 41 55 41 54 56 57 55 53 48 83 ec 28 48 89 cf 48 8d 0d ? ? ? ? e8", 0, Kind::Function };

	// void SetPath(fiDeviceRelative* device, const char* path, bool allowRoot, fiDevice* parent). Xbox RVA 0x28cb210.
	inline constexpr Pattern DeviceSetPath = { "fiDeviceRelative::SetPath",
		"41 56 56 57 53 48 81 ec ? ? ? ? 4c 89 cf 44 89 c3", 0, Kind::Function };

	// fiDeviceRelative* ctor(void* memory). Object is 0x118 bytes on Xbox. Xbox RVA 0x165fa50.
	// Other constructors share its shape, so it's found through the game's own `new(0x118)` + ctor call in the
	// mount-list code: RipOperand on that call's rel32 gives the constructor.
	inline constexpr Pattern RelativeDeviceCtor = { "fiDeviceRelative ctor",
		"b9 18 01 00 00 e8 ? ? ? ? 48 85 c0 0f 84 ? ? ? ? 48 89 c1 49 89 c6 e8", 26, Kind::RipOperand };

	// fiDevice* ParentDevice(bool directStorage). Returns the game's global Local device (loose files through the
	// Win32 file API) for false, or the DirectStorage device for true. Xbox RVA 0x2932600.
	inline constexpr Pattern ParentDevice = { "parent device chooser",
		"48 83 ec 28 84 c9 74 0e e8 ? ? ? ? 48 8d 05 ? ? ? ? eb 07 48 8d 05 ? ? ? ? 48 83 c4 28 c3", 0, Kind::Function };

	// void OpenArchive(ArchiveSource** out, const char* path, const void* rpf9Header). Opens an .rpf by path through
	// the device stack. Hooked for logging only. Xbox RVA 0xfc21c0.
	inline constexpr Pattern OpenArchive = { "OpenArchive",
		"41 56 56 57 53 48 83 ec 28 4c 89 c3 49 89 d6 48 89 ce 4d 85 c0", 0, Kind::Function };

	// ---- Found, not used yet (kept for later features and for porting checks) -----------------------------------
	namespace unused
	{
		// fiDevice* GetDevice(const char* path, bool). Xbox RVA 0x968490 (42 callers go through a jmp at 0x968480).
		inline constexpr Pattern GetDevice = { "GetDevice",
			"56 57 53 48 83 ec 30 89 d3 48 89 ce e8 ? ? ? ? 8a 0e", 0, Kind::Function };
		// RPF9 header reader. Xbox RVA 0x1fb2600.
		inline constexpr Pattern ReadRpf9Header = { "RPF9 header reader",
			"41 57 41 56 41 54 56 57 55 53 48 83 ec 70 4d 85 c9", 0, Kind::Function };
		// fiDeviceLocal vtable (Xbox 0x62fdae0): OpenBulk slot 5, GetFileSize 31, GetFileTime 33, GetAttributes 45.
		inline constexpr Pattern LocalOpenBulk = { "fiDeviceLocal::OpenBulk",
			"56 57 48 81 ec ? ? ? ? 48 8d 7c 24 40 4c 89 c6", 0, Kind::Function };
		inline constexpr Pattern LocalGetFileSize = { "fiDeviceLocal::GetFileSize",
			"56 57 53 48 81 ec ? ? ? ? 48 8d 74 24 50 48 89 f1", 0, Kind::Function };
		inline constexpr Pattern LocalGetFileTime = { "fiDeviceLocal::GetFileTime",
			"56 57 48 81 ec ? ? ? ? 48 8d 7c 24 50", 0, Kind::Function };
		inline constexpr Pattern LocalGetAttributes = { "fiDeviceLocal::GetAttributes",
			"56 48 81 ec ? ? ? ? 48 8d 74 24 20 48 89 f1 e8 ? ? ? ? 48 89 f1 ff 15 ? ? ? ? 90", 0, Kind::Function };
		// Per-entry handler of the data-driven mount list. Xbox RVA 0x1d4c7e0.
		inline constexpr Pattern MountVisitor = { "mount-list visitor",
			"41 57 41 56 56 57 53 48 83 ec 30 48 8b 02 48 8d 5c 24 2c", 0, Kind::Function };
	}

	// Scans for a pattern, applies its offset and, for RipOperand, follows the rel32 operand.
	// Returns 0 (and logs the pattern's name) when it isn't found, so callers can skip the feature.
	inline uintptr_t find(const Pattern& p)
	{
		auto m = memory::scan(p.signature, true);
		if (m.address == 0)
		{
			logger::write("info", " !! Pattern not found: %s (feature disabled)", p.name);
			return 0;
		}
		m = m.add(p.offset);
		if (p.kind == Kind::RipOperand)
			m = m.rip();
		return m.address;
	}
}
