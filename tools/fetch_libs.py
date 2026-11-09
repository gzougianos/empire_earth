"""Fetch the vintage Microsoft SDK files the build uses into toolkit/ (gitignored; env EE_TOOLCHAIN moves it). Every
download and every extracted file is sha256-pinned; refuses on a mismatch.

  python -I tools/fetch_libs.py [--libs] [--dl DIR ...]

Default: the DirectX 7.0a SDK headers (toolkit/dx7sdk/include; the DX7 renderer sources compile against them).
--libs also fetches the libraries the link uses: VC6 RTM import libraries and msvcrt.lib (toolkit/vc98lib/rtm),
DirectX 8.1 SDK ddraw/dinput/dxguid.lib (toolkit/dx81sdk/lib) and the Platform SDK (June 2001) IPHlpApi.Lib
(toolkit/psdk/lib).

Downloads come from archive.org and are cached in toolkit/dl/. --dl DIR (repeatable) names directories that
already hold some of the downloads (same file names); they are read, never written. Needs 7-Zip: env EE_7Z, the
portable copy setup.cmd puts in toolkit/7zip/, `7z` on PATH, C:\\Program Files\\7-Zip\\7z.exe or MSYS2's p7zip.
Extraction goes to a fresh temporary directory (workdir/tmp when run from setup.cmd) and nothing from it is executed.
"""
import argparse
import hashlib
import os
import pathlib
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from ee import download

ROOT = pathlib.Path(__file__).resolve().parent.parent
TC = pathlib.Path(os.environ.get("EE_TOOLCHAIN", ROOT / "toolkit"))

DOWNLOADS = {
    "dx7adxf.exe": ("https://archive.org/download/dx7adxf/dx7adxf.exe",
                    "c0b2bb400322a12e66d0c724825be7146583315fc08e54f3a287cfb30c363540"),
    "VCP600ENU1.7z": ("https://archive.org/download/X04-58170/1999-02_X04-58170_X04-58170_VCP600ENU1.7z",
                      "4e3fa95110e263fe4a42fe151b3f5208d414db72c006beee999a76c6c0ed4261"),
    "dx81sdk_full.exe": ("https://archive.org/download/dx81sdk_full/dx81sdk_full.exe",
                         "73f6791e0ae7f8a1d74f71d8ebe517a79c0cbf03fd344424696fd7a3816d2d02"),
    "en_platform_sdk_june_2001.exe": ("https://archive.org/download/en_platform_sdk_june_2001/"
                                      "en_platform_sdk_june_2001.exe",
                                      "c2dae26429bcdc980e2acce518d59bda369f5ae5b004c1bbe39065c9ee7190f2"),
}
DX7_INCLUDE_DIGEST = "73a7a4ae12eb155816fd428c9c0add6288e66e30d13f8d4ca4ccb60ae2999f6d"
LIBS = {
    "vc98lib/rtm/MSVCRT.LIB": "66cde1a18119df60e3770299c41c1150b9bc5efa5125edf6c8e432836211ff13",
    "vc98lib/rtm/KERNEL32.LIB": "cd4a546f71481319aeee455aa0fb51698a094f9d3e6d8339acf445489a263c68",
    "vc98lib/rtm/USER32.LIB": "0a3c34c4daec09c1b9fd163d5d0b2ee9a6cbe0b3e863ae81a1fae3b9f936fd19",
    "vc98lib/rtm/GDI32.LIB": "dc99cf3da3e3e6b2efeee7724a6cb400a615f8e2dde9ebf6df6b3bf3eb3d7c1b",
    "vc98lib/rtm/ADVAPI32.LIB": "88e77c6590be2a9139c0015c1f07965d7c261cc552de4209d6e8e8d86c4c11c4",
    "vc98lib/rtm/OLE32.LIB": "f31f06ecaf4d8c5e9990cd65f752e9cc7466bf3694555c3914219cbfaf2026a9",
    "vc98lib/rtm/WINMM.LIB": "bb098679a802afdbc6fdaac03dfe1ea81453a36f6d4768b1c1cd092b1dbe9342",
    "vc98lib/rtm/IMM32.LIB": "eb5e8621914af650a8717b2d1789084cdc8d795b8c9a1a811969476c5436f161",
    "vc98lib/rtm/WS2_32.LIB": "dbe9de991aea6662d8857da13e572729b5379377be8e13bdd86e57a51354988e",
    "vc98lib/rtm/SHELL32.LIB": "53150052405bbce4f264226a7bd86d3496bafe8c4bc2252835415826d996bd53",
    "vc98lib/rtm/WSOCK32.LIB": "3ac0ae32d0c9ea77813db7f4d2ea8eba23ae420b40a24845cbe36f6702fca4ba",
    "vc98lib/rtm/OLDNAMES.LIB": "348a220e94f90bd9f530514ae59ff9b044b76e2056f52b45df406c12d2156afb",
    "dx81sdk/lib/ddraw.lib": "6e9e94271645a16f5dee111cf380eaea1dd03bafcfab676d4198cfd7900de036",
    "dx81sdk/lib/dinput.lib": "6cefbeaf8688ea42259a055f84feec0410a666be6ddfdcc745c2661edeb123da",
    "dx81sdk/lib/dxguid.lib": "73a147b9ff6e4527e543e9a436cb244adcdae4ad89b89df51a0634f9a8c13682",
    "psdk/lib/IPHlpApi.Lib": "b999ae32e6f3b05e6bef6a8f597fd322a3b540657145c3d1af0617537d3e3586",
}
MSYS_7Z = pathlib.Path(r"C:\msys64\usr\lib\p7zip\7z.exe")


def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def dir_digest(d):
    h = hashlib.sha256()
    for p in sorted(pathlib.Path(d).iterdir(), key=lambda p: p.name.lower()):
        h.update(p.name.lower().encode() + b"\0" + bytes.fromhex(sha(p)))
    return h.hexdigest()


def fetch(name, extra_dirs):
    url, pin = DOWNLOADS[name]
    for d in extra_dirs:
        p = pathlib.Path(d) / name
        if p.exists() and sha(p) == pin:
            return p
    dst = TC / "dl" / name
    if not dst.exists():
        print(f"downloading {url}", flush=True)
        download(url, dst)
    if sha(dst) != pin:
        sys.exit(f"{name}: sha256 {sha(dst)} != pinned {pin}")
    return dst


def sevenzip():
    for c in (os.environ.get("EE_7Z"), TC / "7zip" / "7z.exe", shutil.which("7z"), r"C:\Program Files\7-Zip\7z.exe"):
        if c and pathlib.Path(c).exists():
            return pathlib.Path(c), False
    if MSYS_7Z.exists():
        return MSYS_7Z, True
    sys.exit("7-Zip not found: install it (or MSYS2 p7zip) or set EE_7Z")


def x7(archive, out, *patterns):
    exe, msys = sevenzip()
    env = dict(os.environ)
    if msys:
        env["PATH"] = str(MSYS_7Z.parents[2] / "bin") + os.pathsep + env["PATH"]

    def p(x):
        x = pathlib.Path(x).resolve()
        return f"/{x.as_posix()[0].lower()}{x.as_posix()[2:]}" if msys else str(x)
    subprocess.run([str(exe), "x", "-y", f"-o{p(out)}", p(archive), *patterns], check=True, capture_output=True,
                   env=env)


def put(src, rel):
    dst = TC / rel
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(src, dst)
    if sha(dst) != LIBS[rel]:
        sys.exit(f"{rel}: sha256 {sha(dst)} != pinned {LIBS[rel]}")


def dx7_headers(dl):
    inc = TC / "dx7sdk" / "include"
    if inc.exists() and dir_digest(inc) == DX7_INCLUDE_DIGEST:
        print("dx7sdk headers: ok")
        return
    with tempfile.TemporaryDirectory() as td:
        x7(fetch("dx7adxf.exe", dl), td, "DXF/include/*")
        got = pathlib.Path(td) / "DXF" / "include"
        if dir_digest(got) != DX7_INCLUDE_DIGEST:
            sys.exit("dx7adxf.exe DXF/include: digest mismatch")
        if inc.exists():
            shutil.rmtree(inc)
        shutil.copytree(got, inc)
    print("dx7sdk headers: ok")


def libs(dl):
    if all((TC / rel).exists() and sha(TC / rel) == h for rel, h in LIBS.items()):
        print("libraries: ok")
        return
    with tempfile.TemporaryDirectory() as td:
        td = pathlib.Path(td)
        x7(fetch("VCP600ENU1.7z", dl), td / "vc6")
        mdf = next((td / "vc6").rglob("*.mdf"))
        iso = td / "vc6.iso"
        with open(mdf, "rb") as f, open(iso, "wb") as o:
            while len(b := f.read(2448)) == 2448:
                o.write(b[16:16 + 2048])
        mdf.unlink()
        x7(iso, td / "vc6x", "VC98/LIB/*")
        iso.unlink()
        for rel in [r for r in LIBS if r.startswith("vc98lib/rtm/")]:
            put(td / "vc6x" / "VC98" / "LIB" / pathlib.PurePath(rel).name, rel)
        x7(fetch("dx81sdk_full.exe", dl), td / "dx81", "DXF/DXSDK/lib/*")
        for n in ("ddraw", "dinput", "dxguid"):
            put(td / "dx81" / "DXF" / "DXSDK" / "lib" / f"{n}.lib", f"dx81sdk/lib/{n}.lib")
        x7(fetch("en_platform_sdk_june_2001.exe", dl), td / "psdk", "setup/WinBaseSDK-common.2.cab")
        x7(td / "psdk" / "setup" / "WinBaseSDK-common.2.cab", td / "psdkc",
           "IPHlpApi_Lib.95CE7DC7_A68D_11D2_A852_00C04FC2A854")
        put(td / "psdkc" / "IPHlpApi_Lib.95CE7DC7_A68D_11D2_A852_00C04FC2A854", "psdk/lib/IPHlpApi.Lib")
    print("libraries: ok")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--libs", action="store_true", help="also fetch the link libraries")
    ap.add_argument("--dl", action="append", default=[], help="directory holding downloads already (read only)")
    a = ap.parse_args()
    dx7_headers(a.dl)
    if a.libs:
        libs(a.dl)


if __name__ == "__main__":
    main()
