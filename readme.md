# RageOpenVI

Work in progress: mods-folder RPF loader for GTA VI (PC), based on RageOpenRDR2. Single-player only.

> **Status: not usable yet.** GTA VI has no PC release, so the loader can't be pointed at a real game executable.
> All game-specific byte patterns live in [`src/game/patterns.h`](src/game/patterns.h) and are still the RDR2
> ones inherited from RageOpenRDR2. Until they are found again in a GTA VI PC executable, every pattern reports
> "not found" in the log and its feature turns itself off. Nothing is patched and nothing crashes.

## What it does

RageOpenVI is an `.asi` plugin loaded by an ASI loader. It hooks the game's file-device layer (RAGE `fiDevice`)
so that:

- a lookup for a file inside the game's archives first checks a `mods/` folder next to the game, and loads your
  replacement from there if it exists (results are cached, so lookups stay fast);
- extra devices are mounted from `newmods/` (`platform:/`, `common:/`, their `crc` variants, and `dlcpacks:/`),
  so loose files and add-on DLC packs can be loaded without editing the original archives.

## Single-player only

This is a single-player modding tool. Don't use it, or anything it loads, in any online mode. Online play is
governed by the publisher's terms and anti-cheat, and modified files there can get accounts banned. The plugin
has an `enabled` switch in `RageOpenVI.ini` so it can be turned off without uninstalling it.

## Porting to a GTA VI PC executable

1. Find each function in [`src/game/patterns.h`](src/game/patterns.h) in the new executable. The names say what to
   look for (e.g. `fiPackfile::OpenArchive`, `fiDeviceLocal::OpenBulk`), and the RDR2 patterns show the shape.
2. Update the signature (and `offset` where a pattern points inside a function).
3. Check the `fiDevice` virtual method order in [`src/rage/fiDevice.h`](src/rage/fiDevice.h) against the new
   executable's vtables; the local device implements them, so the order must match.
4. Build with `ReleaseWithLogs`, run, and check `RageOpenVI.log`: every pattern that is still missing is listed by
   name.

## Building

- CMake 3.17 or newer and Visual Studio with the C++ x64 tools.
- `release.bat` builds `BUILD/BIN/Release/RageOpenVI.asi`; `generate_vs2022.bat` creates a VS solution.
- The `ReleaseWithLogs` configuration adds logging inside hot hooks (slower). Turn the log categories on in
  `RageOpenVI.ini`, which is created on first run.

## Configuration (`RageOpenVI.ini`)

```ini
[config]
enabled=true     ; false = load but do nothing
console=false    ; open a console window with the log

[log]
info=false
mods=false
rpf=false
device=false
```

## Credits

- Based on [RageOpenRDR2](https://github.com/Chiheb-Bacha/RageOpenRDR2) by Chiheb-Bacha, the RDR2 counterpart of
  [RageOpenV](https://github.com/Chiheb-Bacha/RageOpenV). The hooking approach, device code and most of the
  source come from that project. Its author asks that their compiled builds aren't redistributed; link to their
  official release instead.
- Hooks use [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.
- `src/utils/inicpp.h` is the inicpp single-header INI library.

## License

GPL-3.0, the same as the project it's based on. See [LICENSE.txt](LICENSE.txt). If you distribute a build, its
source must be available under the same license.

This project isn't affiliated with or endorsed by Rockstar Games or Take-Two Interactive.
