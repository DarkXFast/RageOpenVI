#pragma once

// Every game-specific byte pattern the loader needs, in one place.
//
// STATUS: these are still RageOpenRDR2's patterns for RDR2.exe. GTA VI isn't out on PC yet, so none of them is
// expected to match a GTA VI executable. When a PC build exists, each entry has to be found again in that exe and
// updated here; nothing else in the code should need game-specific bytes.
//
// `offset` is added to the match before use (e.g. to land on a call instruction inside the pattern).
// `kind` says how the result is used, so a missing pattern can be skipped safely instead of crashing.

namespace patterns
{
	enum class Kind
	{
		Function,     // address of a function to call or hook
		CallSite,     // address of an E8 call instruction whose target is replaced
		RipOperand,   // address of a rip-relative disp32 operand; the target is read from it
		Patch,        // bytes to patch in place
	};

	struct Pattern
	{
		const char* name;
		const char* signature;
		int offset;
		Kind kind;
	};

	// main.cpp
	inline constexpr Pattern ShowConsole = { "ShowConsole (nop the call that hides the console)",
		"ff 15 ? ? ? ? 33 c9 ff 15 ? ? ? ? 45 33 c9", 0, Kind::Patch };
	inline constexpr Pattern CacheSkip = { "CacheSkip (je -> jmp)",
		"74 ? e8 ? ? ? ? 48 8b c8 48 8d 15 ? ? ? ? e8 ? ? ? ? 66 83 7b", 0, Kind::Patch };

	// hooks/CustomDevice.cpp
	inline constexpr Pattern InitialMountCall = { "InitialMount call",
		"0f b7 05 ? ? ? ? 48 03 c3 44 88 34 38 66 01 1d", 21, Kind::CallSite };
	inline constexpr Pattern GetDevice = { "fiDevice::GetDevice",
		"48 89 5c 24 ? 48 89 6c 24 ? 48 89 74 24 ? 57 48 83 ec ? 41 8a f0 40 8a ea 41 b8", 0, Kind::Function };
	inline constexpr Pattern OpenArchive = { "fiPackfile::OpenArchive",
		"48 89 5c 24 ? 48 89 54 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8d 6c 24 ? 48 81 ec ? ? ? ? 4c 8b bd", 0, Kind::Function };

	// hooks/FileReadHooks.cpp
	inline constexpr Pattern OpenBulk = { "fiDeviceLocal::OpenBulk",
		"40 53 48 81 ec ? ? ? ? 49 8b d8 48 8d 4c 24 ? 4c 8b c2 ba ? ? ? ? e8 ? ? ? ? 48 83 64 24", 0, Kind::Function };
	inline constexpr Pattern GetFileTime = { "fiDeviceLocal::GetFileTime",
		"48 81 ec ? ? ? ? 4c 8b c2 48 8d 4c 24 ? ba ? ? ? ? e8 ? ? ? ? 48 8b c8 4c 8d 44 24 ? 33 d2 ff 15 ? ? ? ? 85 c0 75 ? 33 c0 eb ? 8b 44 24 38", 0, Kind::Function };
	inline constexpr Pattern GetFileSize = { "fiDeviceLocal::GetFileSize",
		"48 81 ec ? ? ? ? 4c 8b c2 48 8d 4c 24 ? ba ? ? ? ? e8 ? ? ? ? 48 8b c8 4c 8d 44 24 ? 33 d2 ff 15 ? ? ? ? 85 c0 75 ? 33 c0 eb ? 8b 44 24 3c", 0, Kind::Function };
	inline constexpr Pattern GetAttributes = { "fiDeviceLocal::GetAttributes",
		"48 89 5c 24 ? 57 48 81 ec ? ? ? ? 4c 8b c2 48 8d 4c 24 ? ba ? ? ? ? e8 ? ? ? ? 48 8b c8", 0, Kind::Function };

	// hooks/ParseHeaderHook.cpp
	inline constexpr Pattern ParseHeaderCall = { "fiPackfile::ParseHeader call",
		"48 8b ce 88 44 24 ? 4c 89 7c 24 ? e8", 12, Kind::CallSite };

	// rage/fiDevice.cpp
	inline constexpr Pattern DeviceMount = { "fiDevice::Mount",
		"48 89 5c 24 ? 57 48 81 ec ? ? ? ? 44 8a 81 ? ? ? ? 48 8b da 48 8b d1 48 8b f9 48 8b cb", 0, Kind::Function };
	inline constexpr Pattern DeviceSetPath = { "fiDeviceRelative::SetPath",
		"48 89 5c 24 ? 48 89 6c 24 ? 48 89 74 24 ? 57 48 81 ec ? ? ? ? 49 8b f1 41 8a e8 48 8b da 48 8b f9 e8", 0, Kind::Function };
	inline constexpr Pattern RelativeDeviceVmt = { "fiDeviceRelative vtable",
		"48 8d 05 ? ? ? ? 48 89 5e ? 48 89 06 eb ? 48 8b f3", 3, Kind::RipOperand };

	// GTA VI candidates, found in the Xbox Series executable (not wired up yet).
	//
	// These are the same engine functions in the GTA VI Xbox build. They're kept here so the PC port starts from
	// GTA VI's code, not RDR2's. Several signatures changed shape (GetDevice takes 2 args, OpenArchive 3), so the
	// hooks above can't simply switch to them: each needs its hook updated first. Check every one against the PC exe.
	namespace gta6_xbox
	{
		// (path, bool) -> fiDevice*. Xbox RVA 0x968490.
		inline constexpr Pattern GetDevice = { "GTA VI GetDevice",
			"56 57 53 48 83 ec 30 89 d3 48 89 ce e8 ? ? ? ? 8a 0e", 0, Kind::Function };
		// (ArchiveSource** out, const char* path, const void* rpf9Header) -> out. Xbox RVA 0xfc21c0.
		inline constexpr Pattern OpenArchive = { "GTA VI OpenArchive",
			"41 56 56 57 53 48 83 ec 28 4c 89 c3 49 89 d6 48 89 ce 4d 85 c0", 0, Kind::Function };
		// RPF9 header reader. Xbox RVA 0x1fb2600.
		inline constexpr Pattern ReadRpf9Header = { "GTA VI RPF9 header reader",
			"41 57 41 56 41 54 56 57 55 53 48 83 ec 70 4d 85 c9", 0, Kind::Function };
		// fiDeviceLocal vtable 0x62fdae0: OpenBulk slot 5, GetFileSize 31, GetFileTime 33, GetAttributes 45.
		inline constexpr Pattern OpenBulk = { "GTA VI fiDeviceLocal::OpenBulk",
			"56 57 48 81 ec ? ? ? ? 48 8d 7c 24 40 4c 89 c6", 0, Kind::Function };
		inline constexpr Pattern GetFileSize = { "GTA VI fiDeviceLocal::GetFileSize",
			"56 57 53 48 81 ec ? ? ? ? 48 8d 74 24 50 48 89 f1", 0, Kind::Function };
		inline constexpr Pattern GetFileTime = { "GTA VI fiDeviceLocal::GetFileTime",
			"56 57 48 81 ec ? ? ? ? 48 8d 7c 24 50", 0, Kind::Function };
		inline constexpr Pattern GetAttributes = { "GTA VI fiDeviceLocal::GetAttributes",
			"56 48 81 ec ? ? ? ? 48 8d 74 24 20 48 89 f1 e8 ? ? ? ? 48 89 f1 ff 15 ? ? ? ? 90", 0, Kind::Function };
		// (device, mountName). Xbox RVA 0x1fb2210.
		inline constexpr Pattern DeviceMount = { "GTA VI fiDevice::Mount",
			"56 57 53 48 83 ec 20 44 8a 81", 0, Kind::Function };
		// (device, path, allowRoot, parent). Xbox RVA 0x28cb210.
		inline constexpr Pattern DeviceSetPath = { "GTA VI fiDeviceRelative::SetPath",
			"41 56 56 57 53 48 81 ec ? ? ? ? 4c 89 cf 44 89 c3", 0, Kind::Function };
		// fiDeviceRelative constructor (object is 0x118 bytes, vtable 0x62fc810). Xbox RVA 0x165fa50. Other
		// constructors share its shape, so it's found through the game's own `new` + ctor call in the mount visitor
		// (`mov ecx, 0x118; call alloc; ...; call ctor`): RipOperand on that call's rel32 gives the ctor.
		inline constexpr Pattern RelativeDeviceCtor = { "GTA VI fiDeviceRelative ctor",
			"b9 18 01 00 00 e8 ? ? ? ? 48 85 c0 0f 84 ? ? ? ? 48 89 c1 49 89 c6 e8", 26, Kind::RipOperand };
		// Per-entry mount visitor; mounts are data-driven, so this replaces RDR2's single InitialMount call.
		inline constexpr Pattern MountVisitor = { "GTA VI mount-list visitor",
			"41 57 41 56 56 57 53 48 83 ec 30 48 8b 02 48 8d 5c 24 2c", 0, Kind::Function };
	}

	// Scans for a pattern, applies its offset and, for RipOperand, follows the rip-relative operand.
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
