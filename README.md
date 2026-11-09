# Empire Earth: The Art of Conquest - source reconstruction

[![build](https://github.com/Protryon/empire_earth/actions/workflows/ci.yml/badge.svg)](https://github.com/Protryon/empire_earth/actions/workflows/ci.yml)

A matching decompilation of *Empire Earth: The Art of Conquest* (Stainless Steel Studios / Sierra, 2002): C++ source
that compiles, with the original toolchain, to the same machine code as the retail release. It covers the game
executable and its first-party DLLs. The source is not complete yet (see Status); every image links from it.

> **This is the `patched` branch**: `fixes-all` plus a self-contained build and, from here on, changes and new
> features of our own. Byte-identity with the retail release is not a goal on this branch; see Branches.

Toolchain of the original build:

- Microsoft Visual C++ 6.0 RTM (C/C++ compiler 12.00.8168), LINK 6.00
- Dinkum C++ Library 2.33 (the game's STL, shipped as `sxlrt233.dll`); its headers are reconstructed in
  `include/dinkum/`
- DirectX 7.0a SDK headers (renderer plug-ins), DirectX 8.1 SDK and Platform SDK (June 2001) libraries
- third-party code linked into the images: IJG libjpeg 6b, libpng 1.0.12, zlib 1.1.3 (`src/ThirdParty/`)

The engine's PKWARE Data Compression Library code (`implode`/`explode`, `src/ThirdParty/pkdcl/`) is not a
reconstruction of the original library: it is a compatible reimplementation of the DCL format written for this
project. It reads and writes the game's compressed data like the original (the game's data files decode identically,
and data compressed by either one decompresses with the other), but its machine code differs from the retail engine's.

This repository contains no game data and no original binaries. You need a legal copy of the game to play anything
you build.

## Modules

| Module | Image | Source |
|---|---|---|
| `exe` | `EE-AOC.exe` (game) | `src/Empire Earth/`, `src/RTS Engine/` |
| `lle` | `Low-Level Engine.dll` (engine core) | `src/Low-Level Engine/` |
| `jpeg` | libjpeg 6b inside `Low-Level Engine.dll` | `src/ThirdParty/libjpeg/` |
| `exeimg` | libpng / zlib / libjpeg inside `EE-AOC.exe` | `src/ThirdParty/` |
| `default` | `Default.dll` (random map generators) | `src/Empire Earth/Random Map/Random Map Generators/Default/` |
| `dx7hr` | `DX7HRDisplay.dll` (renderer) | `src/Low-Level Engine/Graphics Engine/DX7 Display Plug-ins/` |
| `dx7hrtnl` | `DX7HRTnLDisplay.dll` (T&L renderer) | same folder |
| `tcp` | `TCP Protocol.dll` (network) | `src/Low-Level Engine/Network Engine/TCP Protocol/` |
| `mixer` | `Miles Sound System Mixer.dll` (sound) | `src/Low-Level Engine/Sound Engine/Miles Sound System Mixer/` |
| `language` | `Language.dll` (resources only) | `src/Empire Earth/Language/Language.rc` |

## Status

<!-- status:begin -->
| Module | Functions byte-identical | Code bytes byte-identical |
|---|---:|---:|
| EE-AOC.exe (game code) | 15,420 / 16,884 (91.3%) | 1,820,263 / 2,736,250 (66.5%) |
| EE-AOC.exe (WON library) | 8,245 / 8,329 (99.0%) | 1,331,195 / 1,386,583 (96.0%) |
| TCP Protocol.dll | 124 / 125 (99.2%) | 13,417 / 13,649 (98.3%) |
| Miles Sound System Mixer.dll | 106 / 106 (100.0%) | 5,021 / 5,021 (100.0%) |
| Default.dll | 795 / 845 (94.1%) | 52,833 / 97,011 (54.5%) |
| Low-Level Engine.dll | 5,882 / 6,030 (97.5%) | 445,819 / 538,055 (82.9%) |
| libjpeg in the engine | 295 / 295 (100.0%) | 64,136 / 64,136 (100.0%) |
| DX7HRDisplay.dll | 553 / 584 (94.7%) | 28,576 / 60,561 (47.2%) |
| DX7HRTnLDisplay.dll | 594 / 626 (94.9%) | 29,549 / 62,738 (47.1%) |
| libpng/zlib/libjpeg in the exe | 318 / 318 (100.0%) | 98,476 / 98,476 (100.0%) |
| Language.dll | resources only | identical to the original apart from the link time stamps and the debug record (PDB path) |

Measured on 2026-11-07 at source revision 698ba84314bf: a function counts when its compiled code is identical to the original's, in the context of its own unit, with every reference resolving to the same symbol. Counts exclude CRT, SDK and import-thunk code.
<!-- status:end -->

The images link from this source alone. Where a function is not byte-identical yet, the image carries this source's
version of it, so the images are not byte-identical to the retail files. `EE-AOC.exe` includes the WON library from
source. The WON online services themselves were not tested: the servers no longer exist.

Testing (automated, on the full-source build of the game and engine):

- The executable linked entirely from this source plays every tested game identically to the original, compared
  tick by tick (simulation, AI and render state): short games with 2 to 6 AI players on several map sizes and types
  and both difficulty levels, long Deathmatch games to a decisive end, a scripted human player, and save/load in
  every direction (saves are compatible with the original's).
- Multiplayer (TCP/IP, host and client mixing original and rebuilt engines): simulation and AI identical to the end
  of the game, no out-of-sync.
- The original executable running on the rebuilt `Low-Level Engine.dll` plays identically to the retail engine.
- The settings screens and the menu tour pass on both the original and the rebuilt images.
- The multiplayer lobby opens and its LAN and Direct Connect screens behave as on the original. Direct Connect games
  start in every direction between original and rebuilt images. Peer sync during play was not shown there.
- Not covered yet: audio output, physical input devices.

## Building

Everything the build needs and everything it makes stays inside the checkout. Nothing is installed and nothing is
written anywhere else:

| Folder (gitignored) | Contents |
|---|---|
| `toolkit/` | portable Python 3.12 and 7-Zip, the VC6 compiler packages, the SDK headers and link libraries, the download cache (`toolkit/dl/`) |
| `workdir/` | the staged sources (`workdir/EEX/`) and every tool's temporary files (`workdir/tmp/`) |
| `target/` | the built images and their object files; `target/flatten/` holds just the images, side by side |

Requirements: Windows 10 (1803 or later) or 11, whose own `curl.exe`, `tar.exe` and `certutil` the setup uses (the
32-bit VC6 tools run natively on 64-bit Windows), about 3 GB of free disk space while the libraries are extracted
(`toolkit/` is about 1.4 GB afterwards, 1.1 GB of it the download cache in `toolkit/dl/`; a full build adds about
300 MB in `target/`), network access to python.org, github.com and archive.org.

1. Fetch the toolkit (once; safe to re-run). Every download and every extracted file is sha256-pinned and checked.

   ```
   setup.cmd
   ```

   - Python: python.org's embeddable Python 3.12.10, unpacked into `toolkit/python/`. The rest of the setup
     (`tools/setup_toolkit.py`) runs with it.
   - 7-Zip 26.05: `7z.exe` and `7z.dll` unpacked from the official installer into `toolkit/7zip/` (the installer is
     never run).
   - `fetch_toolchain.py`: the decomp.me compiler packages (github.com; about 60 MB): VC6 RTM (the compiler, LINK,
     LIB), SP3 (LIB for one import library), SP4 (libjpeg in the engine, Dinkum's `operator new[]`/`delete[]`),
     SP5 + Processor Pack (Miles' `MSS_auto_cleanup`) and Visual C++ .NET 2002 (only its RC, for the resource
     scripts).
   - `fetch_libs.py`: the DirectX 7.0a SDK headers, and with `--libs` the link libraries: the VC6 RTM import
     libraries and `msvcrt.lib` (from the Visual C++ 6.0 Professional CD), DirectX 8.1 SDK `ddraw.lib`,
     `dinput.lib`, `dxguid.lib` and the Platform SDK (June 2001) `IPHlpApi.Lib` (archive.org; about 1.1 GB of
     downloads, cached in `toolkit/dl/`; `--dl DIR` reads downloads you already have).

   With your own Python 3.12 and 7-Zip instead, run `python -I tools/fetch_toolchain.py 6.0 6.3 6.4 6.5pp 7.0` and
   `python -I tools/fetch_libs.py --libs`; without `--libs` it fetches only the headers, enough for `--compile-only`.

2. Build:

   ```
   build.cmd                  # every image
   build.cmd -j 4 lle         # one image, 4 parallel compiles (default: one per CPU)
   build.cmd --compile-only   # compile every unit, link nothing
   ```

   `build.cmd` runs `build.py` with the portable Python; `python -I build.py` takes the same arguments.

   Outputs (each with its `.map` and `.pdb`):

   | Target | Output |
   |---|---|
   | `tcp` | `target/tcp/TCP Protocol.dll` |
   | `mixer` | `target/mixer/Miles Sound System Mixer.dll` |
   | `default` | `target/default/Default.dll` |
   | `lle` | `target/lle/Low-Level Engine.dll` (with libjpeg and the DCL) |
   | `dx7hr` | `target/dx7hr/DX7HRDisplay.dll` |
   | `dx7hrtnl` | `target/dx7hrtnl/DX7HRTnLDisplay.dll` |
   | `language` | `target/language/Language.dll` |
   | `exe` | `target/exe/EE-AOC.exe` (with the image libraries) |

   Every image that links is also copied to `target/flatten/`, all eight side by side (no `.map`, `.pdb` or link
   files), to copy into the game's folder by hand. An image whose link fails is removed from there, so it never holds
   a stale copy; building only some targets refreshes only theirs.

   Objects go to `target/obj/<module>/`; `--out DIR` puts everything under `DIR` instead of `target/`. A full build
   compiles 845 units and links 8 images.

   The engine and TCP DLLs carry a compensation block (appended after the image, never loaded) so their checksum
   matches the retail files, which the game's multiplayer version check compares (`--no-retail-checksum` leaves them
   as linked).

   Paths: sources are staged under `workdir/EEX/` before each compile and every tool runs with `TMP`/`TEMP` set to
   `workdir/tmp/`. The compiler gets plain absolute paths inside the checkout, so the code it generates is equivalent
   to upstream's but not byte-identical: VC6's code generation depends on the lengths of some paths it reads, and
   those lengths now depend on where the checkout is.

   Original layout (upstream's path normalisation, e.g. to compare against the retail code): set
   `EE_ORIG_SRC_ROOT=C:\EEX` (or `X:\EEX` for another drive). Sources are then staged there, the original build tree,
   so that `__FILE__` strings match; unless the checkout path is 23 characters long the build copies `src/`,
   `include/` and the compiler packages it uses to `C:\EEX-root____________` (same drive as the staging root, padded
   to that length) and compiles from there, and it runs every tool with `TMP`/`TEMP` set to
   `C:\EEX-tmp_____________________` (31 characters). This layout writes outside the checkout. It is not guaranteed
   to match upstream byte for byte either: the toolchain folder is `toolkit/` here and `toolchain/` upstream, and the
   Dinkum wrapper headers name it (`include/dinkum/new`, `xstddef`, `xstring`, `xutility`), so the paths the
   compiler reads differ in length.

   Two builds of the same source give the same images apart from link time stamps, PDB paths and signatures, and
   one more field: VC6 names an anonymous namespace after the source file and the time of the compile (in seconds),
   so the RTTI type name of the one class in an anonymous namespace (`EELobbyConnectDialog.cpp`, in `EE-AOC.exe`)
   carries a number that changes with every compile.

CI (GitHub Actions, `windows-latest`) fetches the pinned toolchain and builds every image.

## Branches

- **`main`**: the reconstruction. Its code is byte-identical to the retail release wherever it is matched, and the
  game it builds behaves like the original in every tested game. It keeps the original's bugs on purpose, so it plays
  multiplayer with the original game and shares saves with it.
- **`fixes`**: `main` plus the fixes that keep compatibility with the original: crashes, resource leaks, memory-safety
  guards, local display, interface and audio fixes, and reads of uninitialised data made to read zero. Simulation, AI,
  network traffic and save files stay exactly the original's, so a `fixes` build plays multiplayer with original
  players and loads their saves, and they load its saves. `FIXES.md` on the branch describes every fix.
- **`fixes-all`**: `fixes` plus every other bug fix, including those that change the simulation or the AI. It
  behaves as the game was meant to rather than as it shipped. Play multiplayer only with other `fixes-all` players
  (the simulation differs from the original's, so mixed games go out of sync); saves load where the save format
  allows. Its `FIXES.md` adds every fix beyond `fixes` and the behaviour changes a player will notice.
- **`patched`**: `fixes-all` plus changes of our own: the place to change how the game is built and to add new
  features to it. Byte-identity with the retail release is not a goal here, only a game that works and is easy to
  build and change:
  - The build is self-contained (`setup.cmd`, `build.cmd`; see Building): nothing to install, nothing written
    outside the checkout. It compiles with plain paths inside the checkout, so even before any change of our own its
    code is equivalent to `fixes-all`'s rather than byte-identical, and the output varies with the checkout's path.
  - Every change on top (new features included) may alter the simulation, the network traffic or the save format
    further. Play multiplayer only with players running the same `patched` build: share the built files rather than
    each player building their own. Saves load as on `fixes-all` until a change touches the save format.

`main`, `fixes` and `fixes-all` build with upstream's tools (`python -I build.py`, toolchain in `toolchain/`; see the
README on each branch); `patched` builds with `setup.cmd` and `build.cmd`. To switch, check the branch out and
rebuild with that branch's command:

```
git switch fixes-all
python -I build.py
```

```
git switch patched
build.cmd
```

The fix branches are patch sets on top of `main`: whenever `main` is updated, their commits are replayed onto the new
`main` (the branches are rewritten, so update a local copy with `git fetch` and `git reset --hard origin/<branch>`
rather than a merge). Each fix is one commit that names the bug it fixes (`git log --oneline main..fixes`).
`FIXES.md` on each fix branch, added by the branch's first commit, describes every fix: the function and its address
in the original game, what the original does wrong and what the fix does, grouped by area.

`patched` is in turn a set of commits on top of `fixes-all`. When `fixes-all` is rewritten, replay them onto the new
one with `git rebase --onto origin/fixes-all <old-base> patched`, where `<old-base>` is the `fixes-all` commit
`patched` currently starts from (the parent of its first own commit in `git log --oneline`).

## License

No license is granted. Empire Earth is the property of its rights holders. The third-party libraries under
`src/ThirdParty/` keep their own licenses.
