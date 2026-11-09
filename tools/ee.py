"""Shared paths, module table and compiler flags of the build."""
import os
import pathlib
ROOT = pathlib.Path(__file__).resolve().parent.parent
LINK_DIR = ROOT / "link"
# Everything the build fetches (compiler packages, SDK files, portable Python and 7-Zip, download cache) lives in
# toolkit/, everything it stages and its temporary files in workdir/, its output in target/: all inside the checkout.
# EE_TOOLCHAIN only moves where the fetch scripts put things: the Dinkum wrapper headers (include/dinkum) include the
# compiler's own headers as ../../toolkit/msvc6.0/Include/..., so compiling needs the toolchain at toolkit/.
TOOLCHAIN = pathlib.Path(os.environ.get("EE_TOOLCHAIN", ROOT / "toolkit"))
WORKDIR = ROOT / "workdir"
TARGET = ROOT / "target"
COMPILER = "msvc6.0"
ORIG_SRC_ROOT = pathlib.Path(os.environ.get("EE_ORIG_SRC_ROOT", WORKDIR / "EEX"))
# EE_ORIG_SRC_ROOT at the root of a drive (C:\EEX, the original build tree) selects the original layout: the compiler
# then sees the same paths, path lengths and drive-relative include flags as the retail build, so the output matches
# upstream. Anywhere else the build compiles with plain absolute paths inside the checkout; the code is equivalent
# but not byte-identical to the original's.
ORIGINAL_LAYOUT = ORIG_SRC_ROOT.parent == pathlib.Path(ORIG_SRC_ROOT.anchor)
MODULE_SRC = {
    "tcp": "Low-Level Engine/Network Engine/TCP Protocol",
    "mixer": "Low-Level Engine/Sound Engine/Miles Sound System Mixer",
    "default": "Empire Earth/Random Map/Random Map Generators/Default",
    "lle": "Low-Level Engine",
    "dx7hr": "Low-Level Engine/Graphics Engine/DX7 Display Plug-ins",
    "dx7hrtnl": "Low-Level Engine/Graphics Engine/DX7 Display Plug-ins",
    "exe": "",
    "jpeg": "ThirdParty/libjpeg",
    "exeimg": "ThirdParty",
    "won": "Empire Earth/WONAPI",
}
EEX_INCLUDE_DIRS = ["Low-Level Engine/Thread System", "Low-Level Engine/Graphics Engine",
                    "Empire Earth/Database", "Empire Earth/AI Thread"]


def github_asset(repo, asset_id):
    """The GitHub API route to a release asset: a fallback source for download() when the release's download link
    fails (github.com's releases/download endpoint answers 504 for long stretches at times)."""
    return f"https://api.github.com/repos/{repo}/releases/assets/{asset_id}", {"Accept": "application/octet-stream"}


def download(sources, dst, tries=10):
    """Download to dst through dst.part, retrying transient failures (HTTP 5xx, dropped connections) and taking the
    sources in turn. A source is a URL or a (URL, headers) pair; pass one or a list. The caller checks the digest."""
    import shutil
    import time
    import urllib.error
    import urllib.request
    if isinstance(sources, (str, tuple)):
        sources = [sources]
    sources = [s if isinstance(s, tuple) else (s, {}) for s in sources]
    dst = pathlib.Path(dst)
    part = dst.with_name(dst.name + ".part")
    dst.parent.mkdir(parents=True, exist_ok=True)
    for i in range(tries):
        url, headers = sources[i % len(sources)]
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=120) as r, \
                    open(part, "wb") as f:
                shutil.copyfileobj(r, f, 1 << 20)
            part.replace(dst)
            return dst
        except OSError as e:
            if (isinstance(e, urllib.error.HTTPError) and e.code < 500) or i == tries - 1:
                raise
            print(f"{url}: {e}; retrying", flush=True)
            time.sleep(min(5 * (i // len(sources) + 1), 30))


def module_src(mod):
    return ROOT / "src" / MODULE_SRC.get(mod, mod)


MODULE_FLAGS = {
    "tcp": "-O1 -G6 -MD -GR",
    "mixer": "-O1 -G6 -MD -GR",
    "default": "-O1 -G6 -MD -GR",
    "lle": "-O1 -G6 -MD -GR -DLOWLEVELENGINE_EXPORTS -Zi",
    "dx7hr": "-O1 -G6 -MD -GR -DLLE_CURRENT_HEADERS -Zi",
    "dx7hrtnl": "-O1 -G6 -MD -GR -DLLE_CURRENT_HEADERS -Zi",
    "exe": "-O1 -G6 -MD -GR -DLLE_CURRENT_HEADERS -DLLE_EXE_HEADERS -Zi -Zm200",
    "jpeg": "-O1 -G6 -MD",
    "exeimg": "-O2 -MD",
    "won": "-O2 -MD -GR -GX -DEE_DINKUM_SXL233",
}
MODULE_FILES = {"exe": "EE-AOC.exe", "lle": "Low-Level Engine.dll", "default": "Default.dll", "tcp": "TCP Protocol.dll",
                "dx7hr": "DX7HRDisplay.dll", "dx7hrtnl": "DX7HRTnLDisplay.dll",
                "mixer": "Miles Sound System Mixer.dll", "jpeg": "Low-Level Engine.dll", "exeimg": "EE-AOC.exe",
                "won": "EE-AOC.exe"}


def module_filename(mod):
    if mod not in MODULE_FILES:
        raise SystemExit(f"unknown module {mod!r}; known: {', '.join(MODULE_FILES)}")
    return MODULE_FILES[mod]


def module_units(mod):
    uf = LINK_DIR / mod / "units.txt"
    if not uf.exists():
        return [(None, None, p) for p in sorted(module_src(mod).glob("*.cpp"))]
    units = []
    for line in open(uf):
        line = line.split("#", 1)[0].strip()
        if line:
            lo, hi, name = line.split(None, 2)
            units.append((int(lo, 16), int(hi, 16), module_src(mod) / name.strip()))
    return units
