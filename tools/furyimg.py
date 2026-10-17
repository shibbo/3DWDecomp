#!/usr/bin/env python3
"""Load build/fury.elf and expose the game image for analysis / splitting.

Wraps pyelftools with the lookups the splitter needs: address -> symbol,
name -> symbol, the .text/.rodata/.data/.bss ranges, the GOT, and the dynamic
relocations (ABS64 / RELATIVE / GLOB_DAT) keyed by target address.

The binary's .dynsym only has exported symbols.  Local functions (anonymous
namespace code, nerves, static functions) are named from the function map
(config/<ver>/function_map.tsv) and local data from config/<ver>/symbols.txt;
pass those in with `extra_map` / `extra_syms` and they become ordinary
(STB_LOCAL) symbols.

Parsing the ELF with pyelftools takes a few seconds, so the parsed tables are
cached next to the ELF (`<elf>.cache.pkl`, invalidated by the ELF's size/mtime).
"""

from __future__ import annotations

import bisect
import os
import pickle
from pathlib import Path
from typing import NamedTuple

R_AARCH64_ABS64 = 257
R_AARCH64_GLOB_DAT = 1025
R_AARCH64_JUMP_SLOT = 1026
R_AARCH64_RELATIVE = 1027

BASE = 0x7100000000       # load address used by IDA / the function map
_CACHE_VERSION = 4


class Symbol(NamedTuple):
    name: str
    addr: int
    size: int
    type: str        # 'STT_FUNC' | 'STT_OBJECT' | ...
    bind: str        # 'STB_GLOBAL' | 'STB_WEAK' | 'STB_LOCAL'
    section: str     # '.text' | '.rodata' | '.data' | '.bss' | 'UND' | 'ABS'


def _parse_elf(path: str) -> dict:
    from elftools.elf.elffile import ELFFile
    from elftools.elf.sections import SymbolTableSection
    from elftools.elf.relocation import RelocationSection

    out: dict = {}
    with open(path, "rb") as f:
        elf = ELFFile(f)
        ranges = {}
        for nm in (".text", ".rodata", ".data", ".bss"):
            s = elf.get_section_by_name(nm)
            ranges[nm] = (s["sh_addr"], s["sh_addr"] + s["sh_size"])
        out["ranges"] = ranges
        extra = {}
        for nm in (".got", ".dynamic", ".init_array", ".fini_array"):
            s = elf.get_section_by_name(nm)
            if s is not None:
                extra[nm] = (s["sh_addr"], s["sh_addr"] + s["sh_size"])
        out["extra_ranges"] = extra
        out["sec_data"] = {nm: elf.get_section_by_name(nm).data() for nm in (".text", ".rodata", ".data")}

        syms = []
        dynsym = elf.get_section_by_name(".dynsym")
        for sym in dynsym.iter_symbols():
            if not sym.name:
                continue
            shn = sym["st_shndx"]
            syms.append((sym.name, sym["st_value"], sym["st_size"], sym["st_info"]["type"],
                         sym["st_info"]["bind"], shn if isinstance(shn, str) else "DEF"))
        out["dynsym"] = syms

        relocs = {}
        for sec in elf.iter_sections():
            if not isinstance(sec, RelocationSection):
                continue
            symsec = elf.get_section(sec["sh_link"])
            for r in sec.iter_relocations():
                info_sym = r["r_info_sym"]
                nm = ""
                if isinstance(symsec, SymbolTableSection) and info_sym:
                    nm = symsec.get_symbol(info_sym).name
                relocs[r["r_offset"]] = (r["r_info_type"], nm, r["r_addend"])
        out["dyn_relocs"] = relocs
    return out


def _write_cache(path: Path, value) -> None:
    """Publish an optional cache without failing when Windows readers hold it open."""
    tmp = path.with_suffix(".tmp%d" % os.getpid())
    try:
        with tmp.open("wb") as fh:
            pickle.dump(value, fh, protocol=pickle.HIGHEST_PROTOCOL)
        os.replace(tmp, path)
    except OSError:
        # The caller already has the parsed data; cache publication is best effort.
        pass
    finally:
        try:
            tmp.unlink(missing_ok=True)
        except OSError:
            pass


def _load_cached(path: str) -> dict:
    st = os.stat(path)
    key = (_CACHE_VERSION, st.st_size, st.st_mtime_ns)
    cache = Path(str(path) + ".cache.pkl")
    if cache.is_file():
        try:
            with cache.open("rb") as fh:
                ck, data = pickle.load(fh)
            if ck == key:
                return data
        except Exception:
            pass
    data = _parse_elf(path)
    _write_cache(cache, (key, data))
    return data


def read_function_map(path: str) -> list[tuple[int, str]]:
    """(addr, mangled name) for every row of a function-map TSV.  Addresses
    in the map are absolute (0x71xxxxxxxx); returned addresses are image
    offsets like the ELF's."""
    out = []
    with open(path, encoding="utf-8", errors="replace") as fh:
        header = fh.readline().rstrip("\n").split("\t")
        try:
            ai = header.index("Start Address")
            ni = header.index("Mangled Name")
        except ValueError:
            ai, ni = 0, 4
        for line in fh:
            cols = line.rstrip("\n").split("\t")
            if len(cols) <= max(ai, ni) or not cols[ai].strip():
                continue
            addr = int(cols[ai], 16)
            if addr >= BASE:
                addr -= BASE
            name = cols[ni].strip()
            if name:
                out.append((addr, name))
    return out


def read_symbols_txt(path: str) -> list[tuple[int, int, str, str]]:
    """config/<ver>/symbols.txt: `<addr> <size> <type> <name>` per line where
    type is func|object; '#' comments.  Returns (addr, size, type, name)."""
    out = []
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = line.split(None, 3)
            if len(parts) != 4:
                raise SystemExit(f"{path}: bad line {raw!r} (want: addr size type name)")
            addr = int(parts[0], 16)
            if addr >= BASE:
                addr -= BASE
            out.append((addr, int(parts[1], 0), parts[2], parts[3]))
    return out


def _stamp(p: str | None):
    if not p or not os.path.isfile(p):
        return None
    st = os.stat(p)
    return (os.path.abspath(p), st.st_size, st.st_mtime_ns)


class FuryImage:
    def __init__(self, path="build/fury.elf", extra_map: str | None = None,
                 extra_syms: str | None = None):
        # the symbol index (with the map's locals) is cached per input set
        key = (_CACHE_VERSION, _stamp(path), _stamp(extra_map), _stamp(extra_syms))
        idx_cache = Path(str(path) + ".index.pkl")
        if idx_cache.is_file():
            try:
                with idx_cache.open("rb") as fh:
                    ck, state = pickle.load(fh)
                if ck == key:
                    self.__dict__.update(state)
                    return
            except Exception:
                pass
        self._build(path, extra_map, extra_syms)
        state = dict(self.__dict__)
        _write_cache(idx_cache, (key, state))

    def _build(self, path, extra_map, extra_syms):
        d = _load_cached(path)
        self.ranges: dict[str, tuple[int, int]] = d["ranges"]
        self.extra_ranges: dict[str, tuple[int, int]] = d["extra_ranges"]
        self.got_range = self.extra_ranges.get(".got", (0, 0))
        self._sec_data: dict[str, bytes] = d["sec_data"]
        self.dyn_relocs: dict[int, tuple[int, str, int]] = d["dyn_relocs"]

        self.symbols: list[Symbol] = []
        self._by_name: dict[str, Symbol] = {}
        for name, addr, size, typ, bind, shn in d["dynsym"]:
            if shn == "SHN_UNDEF":
                sec = "UND"
            elif shn == "SHN_ABS":
                sec = "ABS"
            else:
                sec = self._section_of(addr)
            self._add(Symbol(name, addr, size, typ, bind, sec))

        if extra_syms and Path(extra_syms).is_file():
            for addr, size, typ, name in read_symbols_txt(extra_syms):
                if name in self._by_name:
                    continue
                t = "STT_FUNC" if typ.lower().startswith("f") else "STT_OBJECT"
                self._add(Symbol(name, addr, size, t, "STB_LOCAL", self._section_of(addr)))

        self._reindex()
        if extra_map and Path(extra_map).is_file():
            self._add_map_locals(read_function_map(extra_map))
            self._reindex()

    # ------------------------------------------------------------------
    def _add(self, s: Symbol) -> None:
        self.symbols.append(s)
        # first definition wins for name lookup; prefer defined over undef
        if s.name not in self._by_name or (self._by_name[s.name].section == "UND" and s.section != "UND"):
            self._by_name[s.name] = s

    def _reindex(self) -> None:
        self._defs = sorted((s for s in self.symbols if s.section in (".text", ".rodata", ".data", ".bss")),
                            key=lambda s: (s.addr, -s.size))
        self._def_addrs = [s.addr for s in self._defs]
        self._text_funcs = sorted({s.addr for s in self._defs if s.section == ".text"})

    def _add_map_locals(self, rows: list[tuple[int, str]]) -> None:
        """Every function-map row whose address has no .text symbol yet becomes a
        local function.  Its size runs to the next known function start, minus
        trailing zero padding."""
        text_lo, text_hi = self.ranges[".text"]
        known = set(self._text_funcs)
        starts = sorted(known | {a for a, _ in rows if text_lo <= a < text_hi})
        for addr, name in rows:
            if addr in known or not (text_lo <= addr < text_hi) or name in self._by_name:
                continue
            i = bisect.bisect_right(starts, addr)
            end = starts[i] if i < len(starts) else text_hi
            code = self.bytes_at(addr, end - addr)
            n = len(code)
            while n >= 4 and code[n - 4:n] == b"\0\0\0\0":
                n -= 4
            self._add(Symbol(name, addr, n, "STT_FUNC", "STB_LOCAL", ".text"))

    def _section_of(self, addr: int) -> str:
        for nm, (lo, hi) in self.ranges.items():
            if lo <= addr < hi:
                return nm
        return "ABS"

    def section_of(self, addr: int) -> str:
        return self._section_of(addr)

    def data_kind(self, addr: int) -> str:
        """Finer classification of a .data address, matching the object-file
        section clang would have put it in: '.data.rel.ro' (before .dynamic),
        '.data' (after the GOT/init arrays) or '.bss'/'.rodata'/'.text'."""
        sec = self._section_of(addr)
        if sec != ".data":
            return sec
        dyn = self.extra_ranges.get(".dynamic")
        if dyn and addr < dyn[0]:
            return ".data.rel.ro"
        return ".data"

    def bytes_at(self, addr: int, size: int) -> bytes:
        nm = self._section_of(addr)
        if nm == ".bss":
            return b"\0" * size
        lo, _ = self.ranges[nm]
        return self._sec_data[nm][addr - lo: addr - lo + size]

    def sym(self, name: str) -> Symbol | None:
        return self._by_name.get(name)

    def syms_at(self, addr: int) -> list[Symbol]:
        i = bisect.bisect_left(self._def_addrs, addr)
        out = []
        while i < len(self._defs) and self._def_addrs[i] == addr:
            out.append(self._defs[i])
            i += 1
        return out

    def defs_in(self, lo: int, hi: int) -> list[Symbol]:
        i = bisect.bisect_left(self._def_addrs, lo)
        j = bisect.bisect_left(self._def_addrs, hi)
        return self._defs[i:j]

    def defs_in_sized(self, lo: int, hi: int, section: str | None = None):
        """Defined symbols in [lo, hi), each with an effective size (a 0-size
        symbol's size is inferred as the gap to the next symbol / hi)."""
        i = bisect.bisect_left(self._def_addrs, lo)
        j = bisect.bisect_left(self._def_addrs, hi)
        out = []
        for k in range(i, j):
            s = self._defs[k]
            if section and s.section != section:
                continue
            nxt = hi
            for m in range(k + 1, len(self._defs)):
                if self._def_addrs[m] > s.addr:
                    nxt = self._def_addrs[m]
                    break
            size = s.size if s.size else max(min(nxt, hi) - s.addr, 0)
            out.append((s, size))
        return out

    def sym_covering(self, addr: int) -> tuple[Symbol, int] | None:
        """Return (symbol, addend) for the defined symbol whose range covers addr."""
        i = bisect.bisect_right(self._def_addrs, addr) - 1
        while i >= 0:
            s = self._defs[i]
            if s.addr == addr:
                # prefer a sized symbol starting exactly here
                j = i
                while j > 0 and self._def_addrs[j - 1] == addr:
                    j -= 1
                cands = [self._defs[k] for k in range(j, i + 1)]
                cands.sort(key=lambda c: (c.bind == "STB_LOCAL", -c.size))
                return cands[0], 0
            span = s.size if s.size else (self._def_addrs[i + 1] - s.addr if i + 1 < len(self._defs) else 1 << 30)
            if s.addr <= addr < s.addr + max(span, 1):
                return s, addr - s.addr
            if s.size and addr >= s.addr + s.size and s.addr < addr - 0x100000:
                break
            i -= 1
        return None

    def got_target(self, got_addr: int) -> tuple[str, int] | None:
        """Resolve a .got slot to (symname, addend) via its dynamic reloc."""
        ent = self.dyn_relocs.get(got_addr)
        if ent is None:
            return None
        rtype, nm, add = ent
        if rtype in (R_AARCH64_GLOB_DAT, R_AARCH64_JUMP_SLOT, R_AARCH64_ABS64) and nm:
            return nm, add
        if rtype == R_AARCH64_RELATIVE:
            cov = self.sym_covering(add)
            if cov:
                return cov[0].name, cov[1]
        return None
