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
