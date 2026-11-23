# RageOpenVI

Work in progress: a mods-folder loader for GTA VI (PC), based on RageOpenRDR2. Single-player only.

> **Status: not usable yet.** GTA VI has no PC release. The byte patterns in
> [`src/game/patterns.h`](src/game/patterns.h) were found in the GTA VI **Xbox Series** executable. The PC build should
> share that engine code, but every pattern still has to be checked against the PC exe. Until then each one reports
> "not found" in the log and its feature turns itself off. Nothing is patched and nothing crashes.

## What it does

RageOpenVI is an `.asi` plugin loaded by an ASI loader. It adds a `mods` folder next to the game executable whose files
take priority over the game's own:

```
<game folder>/mods/<mount point>/<path>
```

For example, a file the game reads as `platform:/data/x.meta` is replaced by `mods/platform/data/x.meta`, and an
archive `platform:/levels/x.rpf` by `mods/platform/levels/x.rpf`. Files that aren't in the folder still come from the
game.

### How

GTA VI keeps a stack of file devices per mount point (`platform:/`, `common:/`, ...). A lookup asks the most recently
mounted device first and moves down while a device reports "not found". RageOpenVI hooks the game's `Mount`. When the
game mounts something and `mods/<mount name>/` exists, it creates a device for that folder **using the game's own
relative-device class** (its constructor and `SetPath`, with the game's Local file device as parent) and mounts it on
top of the stack. It remounts that device on top whenever the game adds another one to the same mount point.

RageOpenRDR2 shipped its own C++ copy of the device class. That only works if every virtual method is in the same
vtable slot as in the game, and GTA VI's device vtable is laid out differently. Using the game's own class avoids that.

Mount names come from the game's data, not the exe. With `device=true` in the ini, every mount the game makes is logged,
which shows the folder names to use. A name ending in `crc` (e.g. `commoncrc:/`) also uses the folder without it
(`mods/common/`).

With `rpf=true`, every archive the game opens by path is logged, with whether it opened.

## Single-player only

This is a single-player modding tool. Don't use it, or anything it loads, in any online mode. Online play is governed by
the publisher's terms and anti-cheat, and modified files there can get accounts banned. The plugin has an `enabled`
switch in `RageOpenVI.ini` so it can be turned off without uninstalling it.

## Porting to the GTA VI PC executable

1. Check each pattern in [`src/game/patterns.h`](src/game/patterns.h) against the PC exe. Each entry has the Xbox RVA
   and a description of the function, so it can be found again if the bytes differ.
2. Check the relative device's size. The game allocates 0x118 bytes on Xbox, and RageOpenVI allocates 0x400 to leave
   room.
3. Build with `ReleaseWithLogs`, run with every log category on, and read `RageOpenVI.log`. Any missing pattern is
   listed by name, the game's mounts are listed with their names, and the mods devices it created are listed.
4. Not ported (RDR2-only): RageOpenRDR2's "keep the console visible" and "cache skip" patches.

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
info=false       ; startup, missing patterns
mods=false       ; mods devices created and mounted
rpf=false        ; archives the game opens
device=false     ; every mount the game makes
```

## Credits

- Based on [RageOpenRDR2](https://github.com/Chiheb-Bacha/RageOpenRDR2) by Chiheb-Bacha, the RDR2 counterpart of
  [RageOpenV](https://github.com/Chiheb-Bacha/RageOpenV). The plugin structure, the mods-folder idea and the
  hooking/memory helpers come from that project. Its author asks that their compiled builds aren't redistributed; link
  to their official release instead.
- Hooks use [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu.
- `src/utils/inicpp.h` is the inicpp single-header INI library.

## License

GPL-3.0, the same as the project it's based on. See [LICENSE.txt](LICENSE.txt). If you distribute a build, its source
must be available under the same license.

This project isn't affiliated with or endorsed by Rockstar Games or Take-Two Interactive.
