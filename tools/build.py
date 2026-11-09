"""Build driver: compile every unit with the pinned VC6 toolchain and link the game's images from source.

  python -I tools/build.py [-j N] [--out DIR] [target ...]                  compile and link
  python -I tools/build.py --compile-only [-j N] [--out DIR] [module ...]   compile only

Targets (default: all): tcp mixer default lle dx7hr dx7hrtnl language exe. Each links into <out>/<target>/ (default
out: target/): TCP Protocol.dll, Miles Sound System Mixer.dll, Default.dll, Low-Level Engine.dll (with libjpeg, module
jpeg), DX7HRDisplay.dll, DX7HRTnLDisplay.dll, Language.dll (resources only) and EE-AOC.exe (with the image libraries,
module exeimg, and the WON library, module won), each with its .map and .pdb. The DLLs link with tools/relink.py, the executable with
tools/exe_fulllink.py (link_exe), the resources compile with tools/rsrc_rebuild.py.

--compile-only modules (default: all): tcp mixer default lle jpeg dx7hr dx7hrtnl exeimg won exe.

Units come from link/<module>/units.txt (in link order), or every .cpp of the module's source folder when the
module has no unit list. Each unit is compiled with the module's flags (tools/ee.py MODULE_FLAGS), or its own
`// UNIT-FLAGS:` line, with the compiler package its `// UNIT-COMPILER:` line names (default VC6 RTM,
toolkit/msvc6.0). Library members built from source (tools/relink.py LIB_UNITS: Dinkum's operator new[]/delete[],
the PKWARE DCL implode/explode, Miles' MSS_auto_cleanup) are compiled with the modules that link them.

Sources are staged under workdir/EEX and every tool runs with TMP/TEMP set to workdir/tmp, so the build writes
nothing outside the checkout (toolkit/ holds the toolchain, target/ the output). Env EE_ORIG_SRC_ROOT moves the
staging folder; set to the root of a drive (C:\\EEX, the original build tree) it selects the original layout, which
reproduces the retail build's compiler input exactly: __FILE__ strings match, a checkout path of other than 23
characters is mirrored to <drive>\\EEX-root____________ and TMP/TEMP point at <drive>\\EEX-tmp_____________________,
because VC6's code generation depends on the lengths of those paths. Objects go to <out>/obj/<module>/<unit
path>.obj. Every unit is compiled once; the link steps take the objects from there. The build reads no original game
file (EE_NO_ORIGINALS=1). Exit status 1 if any unit fails to compile or any link fails.

Retail checksum (default on; --no-retail-checksum turns it off): the game's multiplayer version check
(EEInitializeNetwork) adds up every byte of Low-Level Engine.dll and TCP Protocol.dll (with sxlrt233.dll and the data
files) into a 32-bit checksum, and a host refuses a client whose checksum differs. After the link, each of these two
DLLs gets a compensation block appended after its last section (file overlay: the loader neither maps nor reads it),
whose bytes make the file's byte sum, modulo 2^32 as the game adds it, equal to the retail file's (RETAIL_BYTE_SUM).
The block is 0xff bytes plus one remainder byte. When the linked file's sum is below the retail sum the block is short
(the difference / 255 bytes); when it is above, the sum has to wrap past 2^32 and the block is about 16.8 MB (2^32
/ 255 bytes, less the difference / 255). The PE header checksum is recomputed when the linker set one (neither DLL
has one).
"""
import argparse
import concurrent.futures
import os
import pathlib
import re
import shutil
import sys
import tempfile
import time

os.environ["EE_NO_ORIGINALS"] = "1"
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import relink
import verify
from ee import LINK_DIR, COMPILER, MODULE_FLAGS, ORIGINAL_LAYOUT, ORIG_SRC_ROOT, ROOT, TARGET, TOOLCHAIN, \
    module_units

MODULES = ["tcp", "mixer", "default", "lle", "jpeg", "dx7hr", "dx7hrtnl", "exeimg", "won", "exe"]
TARGETS = ["tcp", "mixer", "default", "lle", "dx7hr", "dx7hrtnl", "language", "exe"]
TARGET_MODULES = {"lle": ["lle", "jpeg"], "exe": ["exe", "exeimg", "won"], "language": []}
PART_OF = {"jpeg": "lle", "exeimg": "exe", "won": "exe"}
LANGUAGE_RC = pathlib.PurePosixPath("Empire Earth/Language/Language.rc")
EXE_MEMBERS = LINK_DIR / "exe" / "fulllink_members.tsv"
MODULE_COMPILER = {"jpeg": "msvc6.4"}
EXTRA_UNITS = {m: ["Low-Level Engine/Utilities/UFtol.cpp"] for m in ("tcp", "default", "dx7hr", "dx7hrtnl")}
LIB_MEMBERS = {m: [n for g in relink.CARVED[m].values() for n in g]
               + [n for _, _, ms in relink.STATIC_LIBS.get(m, []) if isinstance(ms, list) for n in ms]
               for m in relink.CARVED}
LIB_MEMBERS["exe"] = ["newaop", "delaop"]
EXE_LIB_FLAGS = "-O1"
UNIT_COMPILER = re.compile(r"^// UNIT-COMPILER: (\S+)", re.M)
ROOT_PATH_LEN = 23
TMP_PATH_LEN = 31
RETAIL_BYTE_SUM = {"lle": 115122776, "tcp": 1965119}
LIB_DIRS = ["vc98lib", "dx81sdk", "psdk"]


def head(path, n=30):
    return "".join(pathlib.Path(path).read_text(errors="replace").splitlines(keepends=True)[:n])


def unit_compiler(path, default):
    m = UNIT_COMPILER.search(head(path))
    return m.group(1) if m else default


def jobs_for(mod):
    flags = MODULE_FLAGS[mod]
    paths = [p for _, _, p in module_units(mod)]
    paths += [ROOT / "src" / rel for rel in EXTRA_UNITS.get(mod, [])]
    if mod == "won":
        import exe_fulllink
        paths += [p for _, _, p in exe_fulllink.won_extra_units()]
    out = []
    for p in paths:
        if not p.exists():
            sys.exit(f"{mod}: unit {p} does not exist")
        out.append((mod, p, flags, unit_compiler(p, MODULE_COMPILER.get(mod, COMPILER))))
    return out


def lib_jobs(mods):
    out, seen = [], set()
    for m in mods:
        for n in LIB_MEMBERS.get(m, []):
            p = ROOT / "src" / relink.LIB_UNITS[n]
            flags = EXE_LIB_FLAGS if m == "exe" else MODULE_FLAGS[m]
            job = ("lib", p, flags, unit_compiler(p, COMPILER))
            key = cache_key(p, flags, job[3])
            if key not in seen:
                seen.add(key)
                out.append(job)
    return out


def rel_src(path):
    p = pathlib.Path(path).resolve()
    for base in (ROOT / "src", verify.ROOT / "src"):
        if p.is_relative_to(base):
            return pathlib.PurePosixPath(p.relative_to(base).as_posix())
    raise SystemExit(f"{path} is not under src/")


def cache_key(src, flags, pkg):
    return str(rel_src(src)), verify.unit_flags(src, flags), pkg


def obj_path(out, mod, src):
    return out / "obj" / mod / rel_src(src).with_suffix(".obj")


def compile_one(job, out, workroot):
    mod, src, flags, _ = job
    wd = tempfile.mkdtemp(dir=workroot)
    try:
        obj, log = verify.compile_candidate(src.resolve(), flags, wd)
        if obj:
            dst = obj_path(out, mod, src)
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(obj, dst)
        return job, obj is not None, log
    finally:
        shutil.rmtree(wd, ignore_errors=True)


def sync_tree(src, dst):
    want = {p.relative_to(src): p for p in src.rglob("*") if p.is_file()} if src.exists() else {}
    have = {p.relative_to(dst): p for p in dst.rglob("*") if p.is_file()} if dst.exists() else {}
    for rel in have.keys() - want.keys():
        have[rel].chmod(0o666)
        have[rel].unlink()
    for rel, f in want.items():
        g = dst / rel
        st = f.stat()
        if rel in have and g.stat().st_size == st.st_size and g.stat().st_mtime_ns == st.st_mtime_ns:
            continue
        g.parent.mkdir(parents=True, exist_ok=True)
        if rel in have:
            g.chmod(0o666)
        shutil.copy2(f, g)


def staging_drive():
    return pathlib.Path(ORIG_SRC_ROOT.drive + "\\").resolve().drive + "\\"


def canonical_tmp():
    if ORIGINAL_LAYOUT:
        d = pathlib.Path(ORIG_SRC_ROOT.drive + "\\" + "EEX-tmp".ljust(TMP_PATH_LEN - len(ORIG_SRC_ROOT.drive) - 1,
                                                                      "_"))
    else:
        d = ORIG_SRC_ROOT.parent / "tmp"
    d.mkdir(parents=True, exist_ok=True)
    for k in ("TMP", "TEMP"):
        os.environ[k] = str(d)
    tempfile.tempdir = None
    return d


def canonical_root(pkgs):
    # the original layout compiles from a root of the original length with the toolchain at <root>/toolkit (the
    # Dinkum wrapper headers include the compiler's own headers as ../../toolkit/msvc6.0/Include/...)
    if not ORIGINAL_LAYOUT or (len(str(ROOT)) == ROOT_PATH_LEN and TOOLCHAIN == ROOT / "toolkit"):
        return ROOT, TOOLCHAIN
    drive = staging_drive()
    dst = pathlib.Path(drive + "EEX-root".ljust(ROOT_PATH_LEN - len(drive), "_"))
    sync_tree(ROOT / "src", dst / "src")
    sync_tree(ROOT / "include", dst / "include")
    for name in [*pkgs, "dx7sdk", *LIB_DIRS]:
        sync_tree(TOOLCHAIN / name, dst / "toolkit" / name)
    return dst, dst / "toolkit"


def compile_all(jobs, out, n):
    workroot = canonical_tmp()
    for _, src, _, _ in jobs:
        verify.include_paths(verify.stage_source(src))
    objs, failed, done = {}, [], {}
    for pkg in sorted({j[3] for j in jobs}):
        verify.COMPILER = pkg
        batch = [j for j in jobs if j[3] == pkg]
        with concurrent.futures.ThreadPoolExecutor(max(1, n)) as ex:
            for job, ok, log in ex.map(lambda j: compile_one(j, out, workroot), batch):
                mod = job[0]
                done.setdefault(mod, [0, 0])
                done[mod][0 if ok else 1] += 1
                if ok:
                    objs[cache_key(job[1], job[2], pkg)] = obj_path(out, mod, job[1])
                else:
                    failed.append(job)
                    print(f"FAILED {mod} {rel_src(job[1])}\n{log}\n", flush=True)
    verify.COMPILER = COMPILER
    return objs, failed, done


class CompiledObjects:

    def __init__(self, objs, canon, inner):
        self.objs, self.canon, self.inner, self.compiled = objs, canon, inner, []

    def __call__(self, src, flags, workdir):
        key = cache_key(src, flags, verify.COMPILER)
        obj = pathlib.Path(workdir) / "cand.obj"
        if key in self.objs:
            shutil.copy(self.objs[key], obj)
            return obj, ""
        self.compiled.append(key)
        wd = tempfile.mkdtemp(dir=canonical_tmp())
        try:
            got, log = self.inner(self.canon / "src" / key[0], flags, wd)
            if got:
                shutil.copy(got, obj)
            return (obj if got else None), log
        finally:
            shutil.rmtree(wd, ignore_errors=True)


def link_dll(target, out):
    print(f"--- {target}: relink", flush=True)
    return relink.relink(target, out / target)


def byte_sum(data):
    return sum(data) & 0xFFFFFFFF


def pe_checksum(data, at):
    words = bytes(data[:at]) + b"\0\0\0\0" + bytes(data[at + 4:]) + b"\0" * (len(data) & 1)
    s = sum(memoryview(words).cast("H"))
    while s >> 16:
        s = (s & 0xFFFF) + (s >> 16)
    return s + len(data)


def retail_checksum(path, target):
    data = bytearray(pathlib.Path(path).read_bytes())
    before = byte_sum(data)
    pe_at = int.from_bytes(data[0x3C:0x40], "little")
    if data[:2] != b"MZ" or data[pe_at:pe_at + 4] != b"PE\0\0":
        raise SystemExit(f"{path}: not a PE image")
    cks_at = pe_at + 24 + 0x40
    has_cks = data[cks_at:cks_at + 4] != b"\0\0\0\0"
    core = len(data)

    def fill(field):
        data[cks_at:cks_at + 4] = field.to_bytes(4, "little")
        need = (target - byte_sum(data[:core])) & 0xFFFFFFFF
        data[core:] = b"\xff" * (need // 255) + (bytes([need % 255]) if need % 255 else b"")
        return pe_checksum(data, cks_at) - len(data)

    if has_cks:
        field = fill(0) + len(data) + 0x10010
        words = fill(field)
        data += bytes(field - words - len(data))
        if pe_checksum(data, cks_at) != field:
            raise SystemExit(f"{path}: PE checksum {pe_checksum(data, cks_at):#x} != {field:#x}")
    else:
        fill(0)
    block = data[core:]
    if byte_sum(data) != target:
        raise SystemExit(f"{path}: byte sum {byte_sum(data)} != {target}")
    pathlib.Path(path).write_bytes(data)
    return before, len(block)


def compensate(target, out):
    path = out / target / relink.module_filename(target)
    before, n = retail_checksum(path, RETAIL_BYTE_SUM[target])
    print(f"{target}: byte sum {before} -> {RETAIL_BYTE_SUM[target]} (retail), compensation block {n} bytes",
          flush=True)


def link_language(out):
    import rsrc_rebuild
    print("--- language: resources", flush=True)
    dll = rsrc_rebuild.build_dll(ROOT / "src" / LANGUAGE_RC, out / "language", "Language.dll")
    return 0 if dll.exists() else 1


def link_exe(out, objs):
    import exe_fulllink
    print("--- exe: full link", flush=True)
    dst = out / "exe"
    dst.mkdir(parents=True, exist_ok=True)
    items, missing = [], []
    for mod in ("exe", "exeimg", "won"):
        for lo, hi, p in module_units(mod) + (exe_fulllink.won_extra_units() if mod == "won" else []):
            if not p.exists():
                missing.append(p)
                continue
            key = cache_key(p, MODULE_FLAGS[mod], unit_compiler(p, COMPILER))
            items.append({"mod": mod, "lo": lo, "hi": hi, "path": p, "flags": MODULE_FLAGS[mod],
                          "rel": key[0], "obj": objs[key]})
    if missing:
        sys.exit(f"exe: {len(missing)} registered units have no source: {missing[:3]}")
    libobj = {}
    for n in ("newaop", "delaop"):
        p = ROOT / "src" / relink.LIB_UNITS[n]
        libobj[n] = objs[cache_key(p, EXE_LIB_FLAGS, unit_compiler(p, COMPILER))]
    rep, _ = exe_fulllink.link_exe(dst, items, {}, libobj, members=EXE_MEMBERS)
    p2 = rep.get("pass2", {})
    ma = rep.get("member_alias", {"exit": 0, "unresolved": {}, "duplicates_link": []})
    ok = (p2.get("exit") == 0 and not p2.get("unresolved") and not p2.get("duplicates_link")
          and ma["exit"] == 0 and not ma["unresolved"] and not ma["duplicates_link"])
    print(f"exe: {'ok' if ok else 'FAILED'} -> {dst / 'EE-AOC.exe'}", flush=True)
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("names", nargs="*", metavar="target",
                    help=f"default: all ({' '.join(TARGETS)}; --compile-only: {' '.join(MODULES)})")
    ap.add_argument("--compile-only", action="store_true", help="compile every unit, do not link")
    ap.add_argument("-j", type=int, default=os.cpu_count() or 2, help="parallel compiles (default: one per CPU)")
    ap.add_argument("--out", default=str(TARGET), help="output root (default target/)")
    ap.add_argument("--no-retail-checksum", action="store_true",
                    help="leave Low-Level Engine.dll and TCP Protocol.dll as linked (no compensation block; the "
                         "multiplayer version check then refuses retail peers)")
    a = ap.parse_args()
    if a.compile_only:
        mods = a.names or MODULES
        for m in mods:
            if m not in MODULES:
                sys.exit(f"unknown module {m!r}; known: {' '.join(MODULES)}")
        targets = []
    else:
        targets = a.names or TARGETS
        for t in targets:
            if t in PART_OF:
                sys.exit(f"{t} is linked into {PART_OF[t]}: build {PART_OF[t]} (or compile it with --compile-only)")
            if t not in TARGETS:
                sys.exit(f"unknown target {t!r}; known: {' '.join(TARGETS)}")
        targets = [t for t in TARGETS if t in targets]
        mods = [m for t in targets for m in TARGET_MODULES.get(t, [t])]

    jobs = [j for m in mods for j in jobs_for(m)] + lib_jobs(mods)
    pkgs = sorted({j[3] for j in jobs} | ({COMPILER} if targets else set()))
    for pkg in pkgs:
        if not (TOOLCHAIN / pkg / "Bin" / "CL.EXE").exists():
            sys.exit(f"{TOOLCHAIN / pkg} is missing: python -I tools/fetch_toolchain.py {pkg[4:]}")
    if any("DX7 Display Plug-ins" in str(j[1]) or "dx7sdk" in verify.unit_includes(j[1]) for j in jobs) \
            and not (TOOLCHAIN / "dx7sdk" / "include" / "ddraw.h").exists():
        sys.exit(f"{TOOLCHAIN / 'dx7sdk'} is missing: python -I tools/fetch_libs.py")
    if targets:
        import rsrc_rebuild
        need = [rsrc_rebuild.RC_EXE] if {"language", "exe"} & set(targets) else []
        if set(targets) - {"language"}:
            import implib
            need += [TOOLCHAIN / p / "Bin" / "LIB.EXE" for p in {COMPILER, *implib.IMPLIB_PKG.values()}]
        need += [relink.REAL_LIB_DIR / p for t in targets for p in relink.REAL_LIBS.get(t, {}).values()]
        need += [relink.REAL_LIB_DIR / ms for t in targets for _, _, ms in relink.STATIC_LIBS.get(t, [])
                 if isinstance(ms, str)]
        if "exe" in targets:
            import exe_fulllink
            need += [TOOLCHAIN / p for p in exe_fulllink.exe_roundtrip.REAL_IMPORT_LIBS.values()]
        for p in need:
            if not pathlib.Path(p).exists():
                sys.exit(f"{p} is missing: python -I tools/fetch_toolchain.py 6.0 6.3 6.4 6.5pp 7.0 and "
                         f"python -I tools/fetch_libs.py --libs")

    canonical_tmp()
    root, verify.TOOLCHAIN = canonical_root(pkgs)
    verify.ROOT = root
    jobs = [(m, root / "src" / p.resolve().relative_to(ROOT / "src"), f, c) for m, p, f, c in jobs]
    out = pathlib.Path(a.out).resolve()
    t0 = time.time()
    objs, failed, done = compile_all(jobs, out, a.j)
    for m in [*mods, "lib"]:
        ok, bad = done.get(m, [0, 0])
        print(f"{m:9} {ok:4} compiled  {bad:3} failed")
    print(f"{len(jobs) - len(failed)}/{len(jobs)} units compiled in {time.time() - t0:.0f} s -> {out / 'obj'}",
          flush=True)
    if failed or not targets:
        sys.exit(1 if failed else 0)

    hook = verify.compile_candidate = CompiledObjects(objs, root, verify.compile_candidate)
    results = {}
    for t in targets:
        if t == "language":
            results[t] = link_language(out)
        elif t == "exe":
            results[t] = link_exe(out, objs)
        else:
            results[t] = link_dll(t, out)
            if results[t] == 0 and t in RETAIL_BYTE_SUM and not a.no_retail_checksum:
                compensate(t, out)
    if hook.compiled:
        print(f"note: {len(hook.compiled)} source(s) compiled during the link: {hook.compiled[:5]}")
    print()
    for t in targets:
        print(f"{t:9} {'ok' if results[t] == 0 else 'FAILED'}")
    print(f"built in {time.time() - t0:.0f} s -> {out}")
    sys.exit(1 if any(results.values()) else 0)


if __name__ == "__main__":
    main()
