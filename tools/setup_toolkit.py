"""Fill toolkit/ with everything the build needs. setup.cmd runs it with the portable Python it unpacks there.

  setup.cmd        (or: toolkit\\python\\python.exe -I tools\\setup_toolkit.py)

Fetches 7-Zip into toolkit/7zip/ (unpacked from the official installer with Windows' own tar.exe; the installer is
never run), then the VC6 compiler packages (tools/fetch_toolchain.py) and the SDK headers and link libraries
(tools/fetch_libs.py --libs) into toolkit/. Every download is sha256-pinned and cached in toolkit/dl/; temporary files
go to workdir/tmp. Nothing is installed and nothing is written outside the checkout. Safe to re-run: whatever is
already in place is checked and kept.
"""
import hashlib
import os
import pathlib
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from ee import TOOLCHAIN, WORKDIR, download, github_asset

SEVENZIP_URL = "https://github.com/ip7z/7zip/releases/download/26.05/7z2605-x64.exe"
SEVENZIP_ASSET = github_asset("ip7z/7zip", 662536847)  # the same file through the GitHub API (fallback source)
SEVENZIP_SHA = "fd99e86f57d8f429ef5ed045bf98315af962e2309b0f1ba87a147a44c71647d0"
SEVENZIP_FILES = ["7z.exe", "7z.dll", "License.txt"]
COMPILERS = ["6.0", "6.3", "6.4", "6.5pp", "7.0"]
WINDOWS_TAR = pathlib.Path(os.environ.get("SystemRoot", r"C:\Windows")) / "System32" / "tar.exe"


def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def fetch(url, pin, *fallbacks):
    dst = TOOLCHAIN / "dl" / url.rsplit("/", 1)[1]
    if not dst.exists():
        print(f"downloading {url}", flush=True)
        download([url, *fallbacks], dst)
    if sha(dst) != pin:
        sys.exit(f"{dst.name}: sha256 {sha(dst)} != pinned {pin}")
    return dst


def sevenzip():
    dst = TOOLCHAIN / "7zip"
    if not all((dst / f).exists() for f in SEVENZIP_FILES):
        installer = fetch(SEVENZIP_URL, SEVENZIP_SHA, SEVENZIP_ASSET)
        dst.mkdir(parents=True, exist_ok=True)
        subprocess.run([str(WINDOWS_TAR), "-xf", str(installer), "-C", str(dst), *SEVENZIP_FILES], check=True)
    print(f"7-zip: ok ({dst})", flush=True)
    return dst / "7z.exe"


def main():
    tmp = WORKDIR / "tmp"
    tmp.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, TMP=str(tmp), TEMP=str(tmp), EE_TOOLCHAIN=str(TOOLCHAIN), EE_7Z=str(sevenzip()))
    tools = pathlib.Path(__file__).resolve().parent
    for script, *args in (["fetch_toolchain.py", *COMPILERS], ["fetch_libs.py", "--libs"]):
        r = subprocess.run([sys.executable, "-I", str(tools / script), *args], env=env)
        if r.returncode:
            sys.exit(f"{script} failed (exit {r.returncode})")
    print(f"toolkit ready: {TOOLCHAIN}")


if __name__ == "__main__":
    main()
