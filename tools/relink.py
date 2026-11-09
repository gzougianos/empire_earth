"""Link one DLL: its compiled units, library members built from source, the vintage libraries
and by-name import libraries, with LINK 6."""
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import implib
import verify
from ee import COMPILER, MODULE_FLAGS, ORIGINAL_LAYOUT, ROOT, module_filename, module_src, module_units
LINK_FLAGS = {
    "tcp": "/DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /MACHINE:I386 /SUBSYSTEM:WINDOWS /NODEFAULTLIB "
           "/BASE:0x10000000",
    "mixer": "/DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /MACHINE:I386 /SUBSYSTEM:WINDOWS /NODEFAULTLIB "
             "/BASE:0x10000000",
    "default": "/DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /MACHINE:I386 /SUBSYSTEM:WINDOWS /NODEFAULTLIB "
               "/BASE:0x10000000",
    "lle": "/DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /MACHINE:I386 /SUBSYSTEM:WINDOWS /NODEFAULTLIB "
           "/BASE:0x10000000",
    "dx7hr": "/DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /MACHINE:I386 /SUBSYSTEM:WINDOWS /NODEFAULTLIB "
             "/BASE:0x10000000",
    "dx7hrtnl": "/DLL /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF /MACHINE:I386 /SUBSYSTEM:WINDOWS /NODEFAULTLIB "
                "/BASE:0x10000000",
}
CARVED = {
    "tcp": {"sxlrt233": ["delaop", "newaop"]},
    "mixer": {"sxlrt233": ["delaop"], "mss32": ["mssauto"]},
    "default": {"sxlrt233": ["delaop"]},
    "dx7hr": {"sxlrt233": ["delaop", "newaop"]},
    "dx7hrtnl": {"sxlrt233": ["delaop", "newaop"]},
    "lle": {"sxlrt233": ["newaop", "delaop"]},
}
SHARED_UNITS = {m: ["Low-Level Engine/Utilities/UFtol.cpp"] for m in ("tcp", "default", "dx7hr", "dx7hrtnl")}
REAL_LIB_DIR = verify.TOOLCHAIN
_RTM = {n: f"vc98lib/rtm/{n}.LIB" for n in ("MSVCRT", "KERNEL32", "USER32", "GDI32", "ADVAPI32", "OLE32", "WINMM", "IMM32",
                                    "WS2_32")}
REAL_LIBS = {
    "tcp": {**{n: _RTM[n] for n in ("MSVCRT", "WS2_32", "KERNEL32")}, "IPHLPAPI": "psdk/lib/IPHlpApi.Lib"},
    "mixer": {n: _RTM[n] for n in ("MSVCRT",)},
    "default": {n: _RTM[n] for n in ("MSVCRT",)},
    "dx7hr": {**{n: _RTM[n] for n in ("MSVCRT", "KERNEL32", "USER32", "GDI32")}, "DDRAW": "dx81sdk/lib/ddraw.lib"},
    "dx7hrtnl": {**{n: _RTM[n] for n in ("MSVCRT", "KERNEL32", "USER32", "GDI32")}, "DDRAW": "dx81sdk/lib/ddraw.lib"},
    "lle": {**{n: _RTM[n] for n in ("MSVCRT", "KERNEL32", "USER32", "GDI32", "ADVAPI32", "OLE32", "WINMM", "IMM32")},
            "DINPUT": "dx81sdk/lib/dinput.lib"},
}
LIB_UNITS = {"newaop": "ThirdParty/Dinkum/newaop.cpp", "delaop": "ThirdParty/Dinkum/delaop.cpp",
             "mssauto": "ThirdParty/mss/mssauto.c",
             "implode": "ThirdParty/pkdcl/implode.c", "explode": "ThirdParty/pkdcl/explode.c"}
UNIT_COMPILER = re.compile(r"^// UNIT-COMPILER: (\S+)", re.M)


def compile_unit(src, flags, dst, compiler=None):
    m = UNIT_COMPILER.search("".join(pathlib.Path(src).read_text(errors="replace").splitlines(keepends=True)[:30]))
    saved = verify.COMPILER
    verify.COMPILER = m.group(1) if m else compiler or saved
    try:
        with tempfile.TemporaryDirectory() as td:
            obj, log = verify.compile_candidate(pathlib.Path(src).resolve(), flags, td)
            if not obj:
                sys.exit(log)
            shutil.copy(obj, dst)
    finally:
        verify.COMPILER = saved
    return dst


OBJECT_AT = {}
STATIC_LIBS = {
    "dx7hr": [("dxguid", "DDRAW", "dx81sdk/lib/dxguid.lib")],
    "dx7hrtnl": [("dxguid", "DDRAW", "dx81sdk/lib/dxguid.lib")],
    "lle": [("dxguid", "DINPUT", "dx81sdk/lib/dxguid.lib"),
            ("implode", "dxguid", ["implode", "explode"]),
            ("libjpeg", "IMM32", {"module": "jpeg", "compiler": "msvc6.4"})],
}


def run(cmd, cwd, pkg):
    env = dict(os.environ, PATH=str(pkg / "Bin") + os.pathsep + os.environ["PATH"])
    r = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True,
                       creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    return r.returncode, (r.stdout + r.stderr).strip()


def relink(module, out):
    out = pathlib.Path(out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    pkg = verify.TOOLCHAIN / COMPILER
    py = sys.executable
    dll = module_filename(module)

    units = [(lo, p) for lo, _, p in module_units(module) if p.exists()]
    objs, unit_at = [], []
    for lo, u in units:
        objs.append(compile_unit(u, MODULE_FLAGS[module], out / (u.stem + ".obj")))
        unit_at.append(lo)
    for rel in SHARED_UNITS.get(module, []):
        objs.append(compile_unit(ROOT / "src" / rel, MODULE_FLAGS[module], out / (pathlib.Path(rel).stem + ".obj")))
        unit_at.append(1 << 32)
    print(f"compiled {len(objs)} unit(s)")

    tools = pathlib.Path(__file__).resolve().parent
    carve_dir = out / "carve"
    spec = verify.LINK_DIR / module / "carve.txt"
    shutil.rmtree(carve_dir, ignore_errors=True)
    if spec.exists():
        import carve
        if carve.parse_spec(spec):
            subprocess.run([py, "-I", str(tools / "carve.py"), module, str(spec), str(carve_dir)], check=True,
                           capture_output=True)
    (out / "libsrc").mkdir(exist_ok=True)
    member = {}
    for n in [n for g in CARVED[module].values() for n in g] + [
            n for _, _, ms in STATIC_LIBS.get(module, []) if isinstance(ms, list) for n in ms]:
        member[n] = (compile_unit(ROOT / "src" / LIB_UNITS[n], MODULE_FLAGS[module], out / "libsrc" / f"{n}.obj")
                     if n in LIB_UNITS else carve_dir / f"{n}.obj")
    carved = list(member.values())
    for stem, _, ms in STATIC_LIBS.get(module, []):
        if isinstance(ms, dict):
            d = out / "libsrc" / stem
            d.mkdir(exist_ok=True)
            units_txt = verify.LINK_DIR / ms["module"] / "units.txt"
            us = ([p for _, _, p in module_units(ms["module"])] if units_txt.exists()
                  else sorted(module_src(ms["module"]).glob("*.c")))
            ms["objs"] = [compile_unit(u, MODULE_FLAGS[ms["module"]], d / (u.stem + ".obj"), ms.get("compiler"))
                          for u in us]
            carved += ms["objs"]
    lib_dir = out / "lib"
    subprocess.run([py, "-I", str(tools / "implib.py"), module, str(lib_dir), *map(str, objs + carved)],
                   check=True, capture_output=True)
    libs = [lib_dir / f"{pathlib.Path(dll_name).stem}.lib" for dll_name, _ in implib.read_imports(module)]
    real = REAL_LIBS.get(module, {})
    for i, p in enumerate(libs):
        if p.stem.upper() in real:
            libs[i] = REAL_LIB_DIR / real[p.stem.upper()]
            if not libs[i].exists():
                sys.exit(f"{libs[i]} missing: python -I tools/fetch_libs.py --libs")
    for stem, members in CARVED[module].items():
        if stem == "objects":
            continue
        lib = next(p for p in libs if p.stem.lower() == stem.lower())
        if stem.upper() in real:
            sys.exit(f"{module}: carved members {members} would be merged into the real {lib}")
        code, log = run([str(pkg / "Bin" / "LIB.EXE"), "/nologo", f"/OUT:{lib.name}",
                         *[str(member[n]) for n in members], lib.name], lib_dir, pkg)
        if code:
            sys.exit(log)
    for stem, after, members in STATIC_LIBS.get(module, []):
        if isinstance(members, dict):
            lib = lib_dir / f"{stem}.lib"
            code, log = run([str(pkg / "Bin" / "LIB.EXE"), "/nologo", f"/OUT:{lib.name}", *map(str, members["objs"])],
                            lib_dir, pkg)
            if code:
                sys.exit(log)
            i = next(i for i, p in enumerate(libs) if p.stem.lower() == after.lower())
            libs.insert(i + 1, lib)
            continue
        if isinstance(members, str):
            lib = REAL_LIB_DIR / members
            if not lib.exists():
                sys.exit(f"{lib} missing: python -I tools/fetch_libs.py --libs")
            i = next(i for i, p in enumerate(libs) if p.stem.lower() == after.lower())
            libs.insert(i + 1, lib)
            continue
        lib = lib_dir / f"{stem}.lib"
        code, log = run([str(pkg / "Bin" / "LIB.EXE"), "/nologo", f"/OUT:{lib.name}",
                         *[str(member[n]) for n in members]], lib_dir, pkg)
        if code:
            sys.exit(log)
        i = next(i for i, p in enumerate(libs) if p.stem.lower() == after.lower())
        libs.insert(i + 1, lib)
    at = OBJECT_AT.get(module, {})
    for n in CARVED[module].get("objects", []):
        p = member[n]
        if n in at:
            i = sum(1 for lo in unit_at if lo < at[n])
            objs.insert(i, p)
            unit_at.insert(i, at[n])
        else:
            objs.append(p)

    exp = [ln.rsplit(" @", 1) for ln in (verify.LINK_DIR / module / "exports.txt").read_text(encoding="utf-8")
           .splitlines() if ln.strip() and not ln.startswith("#")]
    if not ORIGINAL_LAYOUT:
        # The retail engine exports two __FILE__ string literals ("\eex\Low-Level Engine\Thread Sys...",
        # "C:\EEX\Low-Level Engine\Graphics..."): their names encode the original build path, which only the original
        # layout reproduces. No module imports them, so the in-checkout layout leaves them out; the other exports
        # keep their ordinals.
        path_strings = [name for name, _ in exp if name.startswith("??_C@") and "eex?2" in name.lower()]
        if path_strings:
            print(f"exports: leaving out {len(path_strings)} build-path string literal(s): {path_strings}")
            exp = [(name, ordinal) for name, ordinal in exp if name not in path_strings]
    deffile = out / f"{module}.def"
    long = {name for name, _ in exp if len(name) == 512}
    full = {}
    if long:
        for p in objs:
            for s in verify.Coff(p).syms.values():
                if s["cls"] == 2 and s["sec"] > 0 and len(s["name"]) > 512 and s["name"][:512] in long:
                    full[s["name"][:512]] = s["name"]
    no_list = os.environ.get("EE_DEF_EXPORTS") == "0"
    deffile.write_text(f'LIBRARY "{pathlib.Path(dll).stem}"\n' + ("" if no_list else "EXPORTS\n" +
                       "".join(f"    {full.get(name, name)} @{ordinal}\n" for name, ordinal in exp)))

    target = out / dll
    lpkg = verify.TOOLCHAIN / os.environ.get("EE_LINK_PKG", COMPILER)
    cmd = [str(lpkg / "Bin" / "LINK.EXE"), "/nologo", *LINK_FLAGS[module].split(), f"/DEF:{deffile.name}",
           f"/OUT:{target.name}", f"/PDB:{target.stem}.pdb", f"/MAP:{target.stem}.map",
           *[str(p.relative_to(out)) for p in objs],
           *[str(p.relative_to(out) if p.is_relative_to(out) else p) for p in libs]]
    (out / "link.cmd").write_text("".join(f'"{a}"\n' for a in cmd[1:]))
    code, log = run(cmd, out, lpkg)
    print(log)
    unresolved = log.count("LNK2001") + log.count("LNK2019")
    dups = log.count("LNK2005") + log.count("LNK4006")
    print(f"link exit {code}: {unresolved} unresolved, {dups} duplicates -> {target}")
    return 1 if code or unresolved or dups else 0
