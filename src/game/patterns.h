#pragma once

// Every game-specific byte pattern the loader needs, in one place.
//
// STATUS: these were found in the GTA VI **Xbox Series** executable (RVAs below are from that build). GTA VI isn't
// out on PC yet. The PC build should share the same engine code, but its bytes may differ, so each pattern has to be
// checked against the PC exe. A pattern that doesn't match is logged by name and its feature turns itself off.
//
// `offset` is added to the match before use. `kind` says how the result is used.
//
// Every function the loader needs has a FALLBACK pattern too (namespace `fallback`). The fallbacks don't look at the
// function's own bytes: they find a call to it in a different place (mostly the game's mount-list code) and follow
// the call. So if a function's prologue changes on PC but its callers don't, the feature still works. `find_any`
// tries the main pattern first, then the fallback, and logs when they disagree.
//
// Check all of this offline with `tools/patcheck.py <exe>` (main repo): it scans an exe the way the plugin does.

namespace patterns
{
	enum class Kind
	{
		Function,     // address of a function to call or hook
		CallSite,     // address of an E8 call instruction whose target is replaced
		RipOperand,   // address of a rel32 operand (call target or rip-relative data); the target is read from it
		Patch,        // bytes to patch in place
		Value32,      // address of a 32-bit immediate the plugin reads (e.g. an object size)
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

	// ---- Fallbacks: the same functions, found through a call to them somewhere else -------------------------------
	// Most anchors are in the mount-list handler (Xbox 0x1d4c980), which news a relative device, picks its parent,
	// sets its path and mounts it, in that order.
	namespace fallback
	{
		// `call Mount` after the mount name string is picked (2 matches on Xbox, both calls to Mount).
		// Xbox RVA 0x1fb2210.
		inline constexpr Pattern DeviceMount = { "fiDevice::Mount (fallback)",
			"4c 8d 77 70 48 89 d9 48 85 c0 48 0f 45 d0 80 7f 7f 00 49 0f 48 d6 e8", 23, Kind::RipOperand };

		// The first of two Unmount calls in a mount-owner's cleanup (Xbox 0x2932190). Xbox RVA 0x28cb640.
		inline constexpr Pattern DeviceUnmount = { "fiDevice::Unmount (fallback)",
			"48 85 c0 74 27 48 89 c1 e8 ? ? ? ? 48 8b 4e 10", 9, Kind::RipOperand };

		// `call SetPath(dev, path, allowRoot, parent)` in the mount-list handler. Xbox RVA 0x28cb210.
		inline constexpr Pattern DeviceSetPath = { "fiDeviceRelative::SetPath (fallback)",
			"48 89 d9 48 89 c2 41 89 e8 4d 89 f9 e8", 13, Kind::RipOperand };

		// `call ctor` right after `new`, in the mount-list handler. Xbox RVA 0x165fa50.
		inline constexpr Pattern RelativeDeviceCtor = { "fiDeviceRelative ctor (fallback)",
			"48 89 c1 48 89 c3 e8 ? ? ? ? 4c 8d 74 24 40 31 d2 45 31 c0 4c 89 f1 e8 ? ? ? ? 8a 8f", 7, Kind::RipOperand };

		// `call ParentDevice(entry->useDirectStorage)` in the mount-list handler. Xbox RVA 0x2932600.
		inline constexpr Pattern ParentDevice = { "parent device chooser (fallback)",
			"8a 8f ? ? ? ? e8 ? ? ? ? 40 8a af ? ? ? ? 48 8d 8f", 7, Kind::RipOperand };

		// `call OpenArchive` for packfile mount entries, in the mount-list handler. Xbox RVA 0xfc21c0.
		inline constexpr Pattern OpenArchive = { "OpenArchive (fallback)",
			"48 8d 5c 24 50 48 89 c2 45 31 c0 48 89 d9 e8", 15, Kind::RipOperand };
	}

	// ---- Relative device size: what the game passes to `new` before calling the ctor ---------------------------
	// The plugin allocates kDeviceAlloc (0x400) per mods device. If the PC game's object is bigger than that, mods
	// devices are turned off instead of overrunning the allocation.
	// `mov ecx, size` before `new` + ctor in the mount-list visitor (Xbox 0x1d4c847). Xbox value 0x118.
	inline constexpr Pattern RelativeDeviceSize = { "fiDeviceRelative size",
		"b9 ? ? 00 00 e8 ? ? ? ? 48 85 c0 0f 84 ? ? ? ? 48 89 c1 49 89 c6 e8 ? ? ? ? 48 8b 97", 1, Kind::Value32 };
	namespace fallback
	{
		// The same in the mount-list handler (Xbox 0x1d4c9bf). Xbox value 0x118.
		inline constexpr Pattern RelativeDeviceSize = { "fiDeviceRelative size (fallback)",
			"b9 ? ? 00 00 e8 ? ? ? ? 48 85 c0 0f 84 ? ? ? ? 48 89 c1 48 89 c3 e8 ? ? ? ? 4c 8d 74 24 40 31 d2", 1, Kind::Value32 };
	}

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
		// OpenBulk: Xbox RVA 0x28c8220.
		inline constexpr Pattern LocalOpenBulk = { "fiDeviceLocal::OpenBulk",
			"56 57 48 81 ec ? ? ? ? 48 8d 7c 24 40 4c 89 c6", 0, Kind::Function };
		// GetFileSize: Xbox RVA 0x28c8530.
		inline constexpr Pattern LocalGetFileSize = { "fiDeviceLocal::GetFileSize",
			"56 57 53 48 81 ec ? ? ? ? 48 8d 74 24 50 48 89 f1", 0, Kind::Function };
		// GetFileTime: Xbox RVA 0x28c8580.
		inline constexpr Pattern LocalGetFileTime = { "fiDeviceLocal::GetFileTime",
			"56 57 48 81 ec ? ? ? ? 48 8d 7c 24 50", 0, Kind::Function };
		// GetAttributes: Xbox RVA 0x573d7f0.
		inline constexpr Pattern LocalGetAttributes = { "fiDeviceLocal::GetAttributes",
			"56 48 81 ec ? ? ? ? 48 8d 74 24 20 48 89 f1 e8 ? ? ? ? 48 89 f1 ff 15 ? ? ? ? 90", 0, Kind::Function };
		// Per-entry handler of the data-driven mount list. Xbox RVA 0x1d4c7e0.
		inline constexpr Pattern MountVisitor = { "mount-list visitor",
			"41 57 41 56 56 57 53 48 83 ec 30 48 8b 02 48 8d 5c 24 2c", 0, Kind::Function };
	}

	// Scans for a pattern, applies its offset and, for RipOperand, follows the rel32 operand. For Value32 it returns
	// the address of the value. Returns 0 when it isn't found; `quiet` leaves the logging to the caller.
	inline uintptr_t find(const Pattern& p, bool quiet = false)
	{
		auto m = memory::scan(p.signature, true);
		if (m.address == 0)
		{
			if (!quiet)
				logger::write("info", " !! Pattern not found: %s (feature disabled)", p.name);
			return 0;
		}
		m = m.add(p.offset);
		if (p.kind == Kind::RipOperand)
			m = m.rip();
		return m.address;
	}

	// Main pattern first, then its fallback. If both are found but point to different places, the main one wins and
	// the mismatch is logged (one of them matched the wrong code on this build).
	inline uintptr_t find_any(const Pattern& main, const Pattern& alt)
	{
		uintptr_t a = find(main, true);
		uintptr_t b = find(alt, true);
		if (a && b && a != b)
			logger::write("info", " !! %s: main pattern -> +0x%llx, fallback -> +0x%llx; using the main one", main.name,
				(unsigned long long)(a - memory::base()), (unsigned long long)(b - memory::base()));
		if (a)
			return a;
		if (b)
		{
			logger::write("info", " %s: main pattern not found, using the fallback (+0x%llx)", main.name,
				(unsigned long long)(b - memory::base()));
			return b;
		}
		logger::write("info", " !! Pattern not found: %s, nor its fallback (feature disabled)", main.name);
		return 0;
	}
}
