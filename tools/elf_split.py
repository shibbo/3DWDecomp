#!/usr/bin/env python3
"""Carve a per-translation-unit relocatable object out of build/fury.elf.

Given the section address ranges a TU owns (or an explicit list of symbol
names), copy the bytes into a fresh ELF `.o`, discover the symbols inside, and
reconstruct the AArch64 relocations the linker discarded, so that objdiff can
compare it against what clang produces for our source:

  * BL / B  to another symbol            -> R_AARCH64_CALL26 / R_AARCH64_JUMP26
  * ADRP (+ADD / +LDR / +STR) direct     -> ADR_PREL_PG_HI21 + ADD_ABS_LO12_NC / LDSTn_ABS_LO12_NC
  * ADRP (+LDR) through .got             -> ADR_GOT_PAGE + LD64_GOT_LO12_NC
  * ABS64 pointers in .data/.rodata      -> R_AARCH64_ABS64   (from .rela.dyn)

References to data the linker merged away are rebuilt the way clang emits them
in an object file, so objdiff can pair them with the compiled side:

  * C string literals        -> section `.rodata.str1.1`  (+offset, first-use order)
  * FP/SIMD constant loads   -> `.rodata.cst4` / `.cst8` / `.cst16` (deduplicated)
  * switch jump tables       -> `.rodata` (the table bytes)
  * named data outside the TU (dynsym, or config/<ver>/symbols.txt)
                             -> an undefined reference by name
  * unnamed data             -> undefined `lbl_<address>` (name it in symbols.txt)

Local functions (anonymous namespace, nerves, statics) are not in .dynsym; they
get their names from the function map (--map).

    python tools/elf_split.py --out build/foo.o --range .text:0x6b2540:0x6b256c
    python tools/elf_split.py --out build/foo.o SymbolA SymbolB      # by name
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import capstone
from capstone.arm64 import (ARM64_OP_IMM, ARM64_OP_MEM, ARM64_OP_REG,
                            ARM64_INS_ADRP, ARM64_INS_BL, ARM64_INS_B,
                            ARM64_INS_ADD, ARM64_INS_LDR, ARM64_INS_LDRB,
                            ARM64_INS_LDRH, ARM64_INS_LDRSW, ARM64_INS_LDRSB,
                            ARM64_INS_LDRSH, ARM64_INS_STR, ARM64_INS_STRB,
                            ARM64_INS_STRH, ARM64_INS_LDP, ARM64_INS_STP,
                            ARM64_INS_CMP, ARM64_INS_RET, ARM64_INS_BR)

import elfobj as E
from furyimg import FuryImage, BASE

# AArch64 relocation type numbers
R_CALL26 = 283
R_JUMP26 = 282
R_ADR_PREL_PG_HI21 = 275
R_ADD_ABS_LO12_NC = 277
R_LDST8_ABS_LO12_NC = 278
R_LDST16_ABS_LO12_NC = 284
R_LDST32_ABS_LO12_NC = 285
R_LDST64_ABS_LO12_NC = 286
R_LDST128_ABS_LO12_NC = 299
R_ADR_GOT_PAGE = 311
R_LD64_GOT_LO12_NC = 312
R_ABS64 = 257
R_PREL32 = 261

_LDST_BY_ACCESS = {
    1: R_LDST8_ABS_LO12_NC, 2: R_LDST16_ABS_LO12_NC, 4: R_LDST32_ABS_LO12_NC,
    8: R_LDST64_ABS_LO12_NC, 16: R_LDST128_ABS_LO12_NC,
}
_FIXED_ACCESS = {
    ARM64_INS_LDRB: 1, ARM64_INS_STRB: 1, ARM64_INS_LDRSB: 1,
    ARM64_INS_LDRH: 2, ARM64_INS_STRH: 2, ARM64_INS_LDRSH: 2,
    ARM64_INS_LDRSW: 4,
}
_LDST = (ARM64_INS_LDR, ARM64_INS_LDRB, ARM64_INS_LDRH, ARM64_INS_LDRSW, ARM64_INS_LDRSB,
         ARM64_INS_LDRSH, ARM64_INS_STR, ARM64_INS_STRB, ARM64_INS_STRH)

_MD = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_LITTLE_ENDIAN)
_MD.detail = True


_SEC_FLAGS = {
    ".text": E.SHF_ALLOC | E.SHF_EXEC,
    ".rodata": E.SHF_ALLOC,
    ".data.rel.ro": E.SHF_ALLOC | E.SHF_WRITE,
    ".data": E.SHF_ALLOC | E.SHF_WRITE,
    ".bss": E.SHF_ALLOC | E.SHF_WRITE,
}
SHF_MERGE, SHF_STRINGS = 0x10, 0x20


def _primary(group):
    """Pick the symbol a group of same-address aliases is emitted under: clang
    emits C1/D1 as aliases of the base-object C2/D2, so prefer those."""
    def key(s):
        n = s.name
        base_obj = ("C2E" in n or "D2E" in n)
        return (not base_obj, s.bind == "STB_LOCAL", n)
    return sorted(group, key=key)[0]


def _is_c_string(data: bytes) -> bool:
    end = data.find(b"\0")
    if end < 0:
        return False
    s = data[:end]
    if not s:
        return True
    try:
        txt = s.decode("utf-8")
    except UnicodeDecodeError:
        return False
    return all(c.isprintable() or c in "\t\n\r\x1b" for c in txt)


class Carver:
    def __init__(self, img: FuryImage):
        self.img = img
        self.o = E.ElfObject()
        self.placed: dict[str, E.Section] = {}         # symname -> its section (offset 0)
        self.included: set[str] = set()
        self.undefs: set[str] = set()
        self.anon: dict[int, tuple[str, int]] = {}      # src addr -> (section, offset)
        self.sizes: dict[str, int] = {}                 # symname -> effective size
        self.ranges: list[tuple[str, int, int]] = []    # (section, lo, hi) owned by this TU
        self.funcs: list[tuple[str, int, int]] = []     # (primary name, addr, size) of carved code
        self.datas: list[tuple[str, int, int]] = []     # same for carved .rodata/.data objects
        self.offs: dict[str, int] = {}                  # symname -> offset in its section
        self._sec_base: dict[str, int] = {}             # shared section -> address of offset 0
        self._pools: dict[str, tuple[E.Section, dict[bytes, int]]] = {}
        self.ours_refs: dict[str, set[str]] = {}
        self.ours_str_align: dict[bytes, int] = {}
        self.ours_pools: dict[str, bytes] = {}
        self.warnings: list[str] = []
        self.ours: OursObject | None = None
        self.aliases: dict[str, list[str]] = {}         # primary function name -> all its names

    def _in_ranges(self, addr: int) -> bool:
        return any(lo <= addr < hi for _, lo, hi in self.ranges)

    # -- placement -------------------------------------------------------
    def _place_group(self, group, size):
        """Strong symbols of a kind share one section (`.text`, `.data`, ...)
        at their original relative offsets, like an object compiled without
        -ffunction-sections; weak (inline/template) code keeps its own
        COMDAT-style `.text.<name>` section, as the compiler emits it."""
        s0 = _primary(group)
        kind = self.img.data_kind(s0.addr)
        weak = all(s.bind == "STB_WEAK" for s in group)
        # Linking can promote an inline definition to a strong exported symbol.
        # Recover its per-function section from the matching source symbol's
        # linkage; all instructions, sizes and branch destinations remain binary-derived.
        if self.ours is not None and any(
                s.name in self.ours.weak_functions and s.name not in self.ours.interior_call_targets
                for s in group):
            weak = True
        typ = E.SHT_NOBITS if kind == ".bss" else E.SHT_PROGBITS
        name = f"{kind}.{s0.name}" if weak else kind
        sec = self.o.section(name, typ, _SEC_FLAGS[kind], align=16 if kind == ".text" else 8)
        cur = sec.nobits_size if kind == ".bss" else len(sec.data)
        if not weak:
            base = self._sec_base.setdefault(name, s0.addr - cur)
            want = s0.addr - base
            if want > cur:
                if kind == ".bss":
                    sec.nobits_size = want
                else:
                    sec.data += self.img.bytes_at(base + cur, want - cur)
                cur = want
        off = cur
        if kind == ".bss":
            sec.nobits_size = off + size
        else:
            sec.data += self.img.bytes_at(s0.addr, size)
        for s in sorted(group, key=lambda s: s is not s0):
            if s.name in self.included:
                continue
            self.placed[s.name] = sec
            self.offs[s.name] = off
            self.sizes[s.name] = size
            self.included.add(s.name)
            bind = {"STB_WEAK": E.STB_WEAK, "STB_LOCAL": E.STB_LOCAL}.get(s.bind, E.STB_GLOBAL)
            self.o.define(s.name, sec, off, s.size or size,
                          typ=E.STT_FUNC if kind == ".text" else E.STT_OBJECT, bind=bind)
        if kind == ".text":
            self.funcs.append((s0.name, s0.addr, size))
            self.aliases[s0.name] = [s0.name] + [s.name for s in group if s is not s0]
        elif kind != ".bss":
            self.datas.append((s0.name, s0.addr, size))

    def include(self, names: list[str]):
        syms = []
        for n in names:
            s = self.img.sym(n)
            if s is None:
                raise SystemExit(f"unknown symbol: {n}")
            if s.section not in (".text", ".rodata", ".data", ".bss"):
                raise SystemExit(f"{n}: not a text/data symbol (section {s.section})")
            syms.append(s)
        for s in sorted(syms, key=lambda s: s.addr):
            self._place_group([s], s.size)

    def include_ranges(self, ranges: list[tuple[str, int, int]]):
        """ranges: list of (section, start, end).  Every defined symbol that
        falls in a range is carved out (in address order); symbols sharing an
        address are emitted as aliases in one section."""
        self.ranges = ranges
        for base, lo, hi in ranges:
            if base not in (".text", ".rodata", ".data", ".bss"):
                raise SystemExit(f"bad section in range: {base}")
            got = self.img.defs_in_sized(lo, hi, section=base)
            if not got:
                self.warnings.append(f"no symbols in {base} [{lo:#x}, {hi:#x})")
            groups: dict[int, list] = {}
            sizes: dict[int, int] = {}
            for s, size in got:
                groups.setdefault(s.addr, []).append(s)
                sizes[s.addr] = max(sizes.get(s.addr, 0), size)
            covered = lo
            for addr in sorted(groups):
                if addr > covered and base == ".text":
                    gap = self.img.bytes_at(covered, addr - covered)
                    if gap.strip(b"\0"):
                        self.warnings.append(f"unnamed code at {covered + BASE:#x}..{addr + BASE:#x} "
                                             f"(add it to the function map)")
                self._place_group(groups[addr], sizes[addr])
                covered = max(covered, addr + sizes[addr])

    # -- pools for merged constants ----------------------------------------
    def _pool(self, name: str, data: bytes, align: int, entsize: int, flags: int) -> tuple[str, int]:
        if name not in self._pools:
            sec = self.o.section(name, E.SHT_PROGBITS, E.SHF_ALLOC | flags, align=align, entsize=entsize)
            self._pools[name] = (sec, {})
            seeds = self.ours.sec_data if self.ours else self.ours_pools
            if name in seeds and flags & SHF_STRINGS:
                # lay the strings out in the order our object has them, so string offsets agree
                seed = seeds[name]
                seen = self._pools[name][1]
                w = max(entsize, 1)
                i = 0
                while i + w <= len(seed):
                    j = i
                    while j + w <= len(seed) and seed[j:j + w] != b"\0" * w:
                        j += w
                    if j + w > len(seed):
                        break
                    seen.setdefault(seed[i:j + w], i)
                    i = j + w
                    while i < len(seed) and i % align and seed[i] == 0:
                        i += 1
                sec.data += seed
            elif name in seeds and flags & SHF_MERGE and entsize:
                # constants too, so their offsets agree with ours
                seed = seeds[name]
                seen = self._pools[name][1]
                for i in range(0, len(seed) - entsize + 1, entsize):
                    seen.setdefault(seed[i:i + entsize], i)
                sec.data += seed
        sec, seen = self._pools[name]
        if data in seen:
            return name, seen[data]
        while len(sec.data) % align:
            sec.data.append(0)
        off = len(sec.data)
        sec.data += data
        seen[data] = off
        return name, off

    def _string(self, addr: int) -> tuple[str, int]:
        raw = self.img.bytes_at(addr, 0x1000)
        end = raw.find(b"\0")
        data = raw[:end + 1]
        # the linker merges strings, so their alignment is lost; when our object puts this
        # string in a more aligned pool (clang raises it for some uses) and the target's
        # address is aligned that much too, follow our object
        al = self.ours_str_align.get(data, 1)
        if al > 1 and (addr + BASE) % al == 0:
            return self._pool(f".rodata.str1.{al}", data, al, 1, SHF_MERGE | SHF_STRINGS)
        return self._pool(".rodata.str1.1", data, 1, 1, SHF_MERGE | SHF_STRINGS)

    def _const(self, addr: int, size: int) -> tuple[str, int]:
        return self._pool(f".rodata.cst{size}", self.img.bytes_at(addr, size), size, size, SHF_MERGE)

    def _jump_table(self, func: str, addr: int, entsize: int, count: int) -> tuple[str, int]:
        name = ".rodata"
        key = (name, addr)
        if key in self.anon:
            return self.anon[key]
        sec = self.o.section(name, E.SHT_PROGBITS, E.SHF_ALLOC, align=max(entsize, 1))
        while len(sec.data) % entsize:
            sec.data.append(0)
        off = len(sec.data)
        sec.data += self.img.bytes_at(addr, entsize * max(count, 1))
        self.anon[key] = (name, off)
        return name, off

    # -- data guided by our compiled object -------------------------------
    _ADD_TYPES = (R_ADD_ABS_LO12_NC,)
    _LDST_TYPES = (R_LDST8_ABS_LO12_NC, R_LDST16_ABS_LO12_NC, R_LDST32_ABS_LO12_NC,
                   R_LDST64_ABS_LO12_NC, R_LDST128_ABS_LO12_NC)

    def _read_string(self, addr: int, width: int) -> bytes:
        raw = self.img.bytes_at(addr, 0x2000)
        i = 0
        while i + width <= len(raw) and raw[i:i + width] != b"\0" * width:
            i += width
        return raw[:i + width]

    def _copy_data(self, secname: str, src: int, size: int, align: int) -> int:
        """Append target bytes [src, src+size) to a data section named like ours, with their
        pointer relocations; returns the offset."""
        if secname.startswith(".bss"):
            sec = self.o.section(secname, E.SHT_NOBITS, E.SHF_ALLOC | E.SHF_WRITE, align=align)
            off = (sec.nobits_size + align - 1) // align * align
            sec.nobits_size = off + size
            return off
        flags = E.SHF_ALLOC | (E.SHF_WRITE if secname.startswith(".data") else 0)
        sec = self.o.section(secname, E.SHT_PROGBITS, flags, align=align)
        while len(sec.data) % align:
            sec.data.append(0)
        off = len(sec.data)
        sec.data += self.img.bytes_at(src, size)
        for a in range(src - src % 8, src + size, 8):
            if a < src:
                continue
            ent = self.img.dyn_relocs.get(a)
            if not ent:
                continue
            rtype, nm, add = ent
            if rtype == R_ABS64 and nm:
                tgt, tadd = nm, add
            elif rtype in (R_ABS64, 1027):
                cov = self.img.sym_covering(add)
                tgt, tadd = (cov[0].name, cov[1]) if cov else (f"lbl_{add + BASE:X}", 0)
            else:
                continue
            if tgt not in self.included:
                self.undefs.add(tgt)
            sec.reloc(off + a - src, tgt, R_ABS64, tadd)
        return off

    def _ref_guided(self, tgt: int, func: str, foff: int, how: str):
        """Resolve an unnamed data reference the way our object does at the same instruction:
        same kind of section (string pool of the right width, constant pool, .rodata table,
        .data.rel.ro array, ...) or the same local symbol, filled with the target's bytes."""
        if self.ours is None:
            return None
        r = self.ours.ref_at(self.aliases.get(func, [func]), foff)
        if r is None:
            return None
        if r.type not in (self._ADD_TYPES if how == "add" else self._LDST_TYPES):
            return None
        key = ("guided", tgt)
        if key in self.anon:
            return self.anon[key]
        out = None
        m = re.fullmatch(r"\.rodata\.str(\d+)\.(\d+)", r.secname) if r.symtype == "STT_SECTION" else None
        if m:
            width, align = int(m.group(1)), int(m.group(2))
            data = self._read_string(tgt, width)
            out = self._pool(r.secname, data, align, width, SHF_MERGE | SHF_STRINGS)
        elif r.symtype == "STT_SECTION" and re.fullmatch(r"\.rodata\.cst(\d+)", r.secname):
            n = int(r.secname[len(".rodata.cst"):])
            out = self._const(tgt, n)
        elif r.symtype == "STT_SECTION" and r.secname:
            size = self.ours.extent(r.secname, r.addend)
            if size <= 0:
                return None
            al = max(self.ours.sec_align.get(r.secname, 8), 1)
            out = (r.secname, self._copy_data(r.secname, tgt, size, al))
        elif r.sym and not r.secname:
            # our object references it by name from elsewhere
            self.undefs.add(r.sym)
            out = (r.sym, r.addend)
        elif r.sym and r.symtype in ("STT_OBJECT", "STT_NOTYPE") and r.secname:
            # a static / vtable our object defines: retain its binding when carving the target bytes
            start = tgt - r.addend
            size = r.size or self.ours.extent(r.secname, r.value)
            if size <= 0 or self.img.sym(r.sym) is not None:
                return None
            if r.sym not in self.included:
                al = max(self.ours.sec_align.get(r.secname, 8), 1)
                off = self._copy_data(r.secname, start, size, al)
                bind = {"STB_GLOBAL": E.STB_GLOBAL, "STB_WEAK": E.STB_WEAK}.get(r.bind, E.STB_LOCAL)
                visibility = {"STV_INTERNAL": 1, "STV_HIDDEN": 2, "STV_PROTECTED": 3}.get(r.visibility, 0)
                self.o.define(r.sym, self.o._by_name[r.secname], off, size, typ=E.STT_OBJECT,
                              bind=bind, visibility=visibility)
                self.included.add(r.sym)
            out = (r.sym, r.addend)
        if out is not None:
            self.anon[key] = out
        return out

    # -- symbol resolution ---------------------------------------------------
    def _ref_sym(self, target_addr: int):
        """A named symbol covering target_addr: (name, addend) or None."""
        cov = self.img.sym_covering(target_addr)
        if cov is None:
            return None
        s, add = cov
        if s.name in self.included:
            return s.name, add
        if (s.size and add < s.size) or add == 0:
            self.undefs.add(s.name)
            return s.name, add
        return None

    def _pick_alias(self, name: str, target_addr: int, caller: str | None,
                    after_new: bool = False, base_ctor: bool = False) -> str:
        """C1/C2 and D1/D2 are one function in the binary; pick the name clang
        references: a ctor right after `operator new` is C1, a ctor called from
        another ctor is the base-object C2; a dtor called from a *different*
        class's dtor is D2, otherwise D1."""
        group = [s.name for s in self.img.syms_at(target_addr) if s.section == ".text"]
        if len(group) < 2 or not caller:
            return name
        ours = self.ours_refs.get(caller, set())
        for g in group:
            if g in ours:
                if g not in self.included:
                    self.undefs.add(g)
                return g
        def split(n):
            m = re.search(r"^(.*?)([CD][0-2])E", n)
            return (m.group(1), m.group(2)) if m else (None, None)
        ccls, ck = split(caller)
        want = None
        kinds = {split(g)[1] for g in group}
        if kinds & {"C1", "C2"}:
            want = "C2" if base_ctor else ("C1" if after_new or ck not in ("C1", "C2") else "C2")
        elif kinds & {"D1", "D2"}:
            tcls = split(group[0])[0]
            want = "D2" if ck in ("D0", "D1", "D2") and tcls != ccls else "D1"
        for g in group:
            if split(g)[1] == want:
                if g not in self.included:
                    self.undefs.add(g)
                return g
        return name

    def _plt_import(self, target_addr: int) -> str | None:
        """The imported function a PLT stub (adrp x16; ldr x17, [x16, #lo]; add x16, x16, #lo;
        br x17) jumps to, e.g. memcpy or acosf - what clang's object references."""
        if self.img.section_of(target_addr) != ".text":
            return None
        code = self.img.bytes_at(target_addr, 16)
        if len(code) < 16 or code[12:16] != bytes.fromhex("20021fd6"):   # br x17
            return None
        insns = list(_MD.disasm(code, target_addr))
        if len(insns) != 4 or insns[0].id != ARM64_INS_ADRP or insns[0].reg_name(insns[0].operands[0].reg) != "x16":
            return None
        mem = [op for op in insns[1].operands if op.type == ARM64_OP_MEM]
        if not mem:
            return None
        got = self.img.got_target(insns[0].operands[1].imm + mem[0].mem.disp)
        return got[0] if got and got[1] == 0 else None

    def _ref_code(self, target_addr: int, caller: str | None = None, after_new: bool = False,
                  base_ctor: bool = False):
        imp = self._plt_import(target_addr)
        if imp:
            self.undefs.add(imp)
            return imp, 0
        r = self._ref_sym(target_addr)
        if r:
            if r[1] == 0:
                return self._pick_alias(r[0], target_addr, caller, after_new, base_ctor), 0
            return r
        nm = f"sub_{target_addr + BASE:X}"
        self.undefs.add(nm)
        return nm, 0

    def _ref_data(self, tgt: int, how: str, access: int, fp: bool, func: str,
                  jt: tuple[int, int] | None, foff: int | None = None):
        """Resolve a data reference.  how: 'add' (address taken) | 'ldst'."""
        r = self._ref_sym(tgt)
        if r:
            return r
        if foff is not None:
            g = self._ref_guided(tgt, func, foff, how)
            if g:
                return g
        sec = self.img.section_of(tgt)
        if sec == ".rodata":
            if how == "ldst" and fp and access in (4, 8, 16):
                return self._const(tgt, access)
            if how == "add" and jt and not jt[2] and not jt[1] and _is_c_string(self.img.bytes_at(tgt, 0x1000)) \
                    and self.img.bytes_at(tgt, 1) != b"\0":
                # a string compared character by character looks like an indexed table, but
                # has no bounds check in front of it (a switch's jump table always does)
                return self._string(tgt)
            if how == "add" and jt:
                ent, count, is_fp = jt
                if is_fp and ent * count in (4, 8, 16):
                    # a small FP lookup table is a mergeable constant, like clang emits it
                    return self._const(tgt, ent * count)
                return self._jump_table(func, tgt, ent, count)
            if how == "add" and _is_c_string(self.img.bytes_at(tgt, 0x1000)):
                return self._string(tgt)
        if sec in (".rodata", ".data", ".bss", ".text"):
            nm = f"lbl_{tgt + BASE:X}"
            self.undefs.add(nm)
            return nm, 0
        return None

    # -- relocation reconstruction -------------------------------------
    def process(self):
        for name, addr, size in list(self.funcs):
            self._recon_text(name, addr, size)
        self._recon_data_abs64()
        for u in sorted(self.undefs - self.included):
            self.o.undef(u)

    def _local_same_section(self, ref_name: str, sec) -> bool:
        """A branch to a (non-weak) function in the same section is resolved by the
        assembler and has no relocation in a compiled object - even a global one."""
        if self.placed.get(ref_name) is not sec:
            return False
        s = self.img.sym(ref_name)
        return s is not None and s.bind != "STB_WEAK"

    def _recon_text(self, name: str, addr: int, size: int):
        sec = self.placed[name]
        base_off = self.offs[name]
        code = self.img.bytes_at(addr, size)
        fn_lo, fn_hi = addr, addr + size
        insns = list(_MD.disasm(code, addr))
        last_call = ""
        for idx, ins in enumerate(insns):
            rel_off = base_off + ins.address - addr
            gid = ins.id
            if gid == ARM64_INS_BL:
                after_new = last_call.startswith(("_Znw", "_Zna"))
                base = after_new and self._stores_other_vtable(insns, idx)
                if base:
                    after_new = False   # a base ctor inside an inlined derived ctor: C2
                ref = self._ref_code(ins.operands[0].imm, name, after_new=after_new, base_ctor=base)
                if not self._local_same_section(ref[0], sec):
                    sec.reloc(rel_off, ref[0], R_CALL26, ref[1])
                last_call = ref[0]
            elif gid == ARM64_INS_B and len(ins.operands) == 1 and ins.operands[0].type == ARM64_OP_IMM \
                    and ins.mnemonic == "b":
                tgt = ins.operands[0].imm
                if fn_lo <= tgt < fn_hi:
                    continue  # local branch
                ref = self._ref_code(tgt, name)
                if not self._local_same_section(ref[0], sec):
                    sec.reloc(rel_off, ref[0], R_JUMP26, ref[1])
            elif gid == ARM64_INS_ADRP:
                self._recon_adrp(sec, name, insns, idx, addr - base_off, addr)

    def _stores_other_vtable(self, insns, idx) -> bool:
        """Right after calling a constructor at insns[idx], does the code load another class's
        vtable (the inlined derived constructor setting its own vtable)?"""
        callee = self.img.sym_covering(insns[idx].operands[0].imm)
        m = re.match(r"^_ZN(.*?)C[12]E", callee[0].name) if callee else None
        if not m:
            return False
        cls = m.group(1)
        for ins in insns[idx + 1: idx + 9]:
            if ins.id == ARM64_INS_BL:
                return False
            if ins.id != ARM64_INS_ADRP:
                continue
            for how, cins, lo12, acc in self._consumers(insns, insns.index(ins), ins.operands[0].reg):
                got = self.img.got_target(ins.operands[1].imm + lo12)
                if got and got[0].startswith("_ZTV") and cls not in got[0]:
                    return True
        return False

    def _consumers(self, insns, idx, reg):
        """Instructions after insns[idx] that use `reg` as an ADD base or a
        memory base with an immediate, until `reg` is overwritten or control
        flow leaves the straight line."""
        out = []
        for j in range(idx + 1, min(idx + 128, len(insns))):
            ins = insns[j]
            ops = ins.operands
            if ins.id == ARM64_INS_ADD and len(ops) >= 3 and ops[1].type == ARM64_OP_REG \
                    and ops[1].reg == reg and ops[2].type == ARM64_OP_IMM:
                out.append(("add", ins, ops[2].imm, 8))
                if ops[0].reg == reg:
                    break
                continue
            if ins.id in _LDST or ins.id in (ARM64_INS_LDP, ARM64_INS_STP):
                mem = [op for op in ops if op.type == ARM64_OP_MEM]
                if mem and mem[0].mem.base == reg and mem[0].mem.index == 0:
                    if ins.id in (ARM64_INS_LDP, ARM64_INS_STP):
                        rn = ins.reg_name(ops[0].reg) or "x"
                        acc = {"w": 4, "s": 4, "x": 8, "d": 8, "q": 16}.get(rn[0], 8)
                    else:
                        acc = _FIXED_ACCESS.get(ins.id)
                        if acc is None:
                            rn = ins.reg_name(ops[0].reg) or "x"
                            acc = {"w": 4, "s": 4, "x": 8, "d": 8, "q": 16, "h": 2, "b": 1}.get(rn[0], 8)
                    out.append(("ldst", ins, mem[0].mem.disp, acc))
                    if ins.id == ARM64_INS_LDP and any(
                            op.type == ARM64_OP_REG and op.reg == reg for op in ops[:2]):
                        break
                    # a load into the base register itself ends its lifetime
                    if ops[0].type == ARM64_OP_REG and ops[0].reg == reg and ins.id not in (ARM64_INS_STR, ARM64_INS_STRB, ARM64_INS_STRH, ARM64_INS_STP):
                        break
                    continue
            # LDP writes both destinations, including when its memory base is unrelated.
            if ins.id == ARM64_INS_LDP and len(ops) >= 2 and any(
                    op.type == ARM64_OP_REG and op.reg == reg for op in ops[:2]):
                break
            # reg redefined?
            if ops and ops[0].type == ARM64_OP_REG and ops[0].reg == reg:
                break
            if ins.id in (ARM64_INS_RET, ARM64_INS_BR) or (ins.id == ARM64_INS_B and ins.mnemonic == "b"):
                break
        return out

    def _jump_table_shape(self, insns, idx, base_reg, fn_hi):
        """If the ADRP+ADD at idx builds a table base (jump table, or a lookup table
        clang made from a switch/select), return (entsize, count, is_fp)."""
        for j in range(idx + 1, min(idx + 10, len(insns))):
            ins = insns[j]
            if ins.id in (ARM64_INS_LDRB, ARM64_INS_LDRH, ARM64_INS_LDRSW, ARM64_INS_LDR):
                mem = [op for op in ins.operands if op.type == ARM64_OP_MEM]
                if mem and mem[0].mem.base == base_reg and mem[0].mem.index != 0:
                    ent = {ARM64_INS_LDRB: 1, ARM64_INS_LDRH: 2, ARM64_INS_LDRSW: 4}.get(ins.id, 4)
                    dst = ins.reg_name(ins.operands[0].reg) or ""
                    is_fp = dst[:1] in ("s", "d", "q")
                    if is_fp:
                        ent = {"s": 4, "d": 8, "q": 16}[dst[0]]
                    elif dst.startswith("x"):
                        ent = 8
                    # bound check: cmp wN, #k ; b.hi -> k+1 entries
                    count = 0
                    idx_reg = ins.reg_name(mem[0].mem.index)[1:]
                    for k in range(j - 1, max(idx - 12, -1), -1):
                        c = insns[k]
                        if c.id == ARM64_INS_CMP and len(c.operands) == 2 and c.operands[1].type == ARM64_OP_IMM \
                                and k < idx:
                            nxt = insns[k + 1].mnemonic if k + 1 < len(insns) else ""
                            count = c.operands[1].imm + (0 if nxt in ("b.hs", "b.cs") else 1)
                            break
                        # a bool index (cset) selects one of two entries
                        if c.mnemonic == "cset" and (c.reg_name(c.operands[0].reg) or "")[1:] == idx_reg:
                            count = 2
                            break
                    return ent, count, is_fp
        return None

    def _recon_adrp(self, sec, func, insns, idx, fn_addr, func_start=None):
        ins = insns[idx]
        page = ins.operands[1].imm
        reg = ins.operands[0].reg
        rel_off = ins.address - fn_addr
        cons = self._consumers(insns, idx, reg)
        if not cons and self.ours is not None and func_start is not None:
            # the consumer is beyond the straight-line window (e.g. a GOT load hoisted far
            # ahead of its use): follow our object's pairing of this ADRP with its LO12 use
            names = self.aliases.get(func, [func])
            pr = self.ours.ref_at(names, ins.address - func_start)
            if pr is not None and pr.type in (R_ADR_GOT_PAGE, R_ADR_PREL_PG_HI21):
                want = R_LD64_GOT_LO12_NC if pr.type == R_ADR_GOT_PAGE else None
                fn_refs = next((self.ours.funcs[n] for n in names if n in self.ours.funcs), {})
                for o2, r2 in sorted(fn_refs.items()):
                    if o2 <= ins.address - func_start or r2.sym != pr.sym:
                        continue
                    if want is not None and r2.type != want:
                        continue
                    j = (o2 + func_start - insns[0].address) // 4
                    if not (0 <= j < len(insns)):
                        continue
                    c = insns[j]
                    mem = [op for op in c.operands if op.type == ARM64_OP_MEM]
                    if mem and mem[0].mem.base == reg and pr.type == R_ADR_GOT_PAGE:
                        self.undefs.add(pr.sym)
                        sec.reloc(rel_off, pr.sym, R_ADR_GOT_PAGE, pr.addend)
                        sec.reloc(c.address - fn_addr, pr.sym, R_LD64_GOT_LO12_NC, pr.addend)
                        return
        if not cons:
            ref = self._ref_sym(page)
            if ref:
                sec.reloc(rel_off, ref[0], R_ADR_PREL_PG_HI21, ref[1])
            return
        how, first, lo12, acc = cons[0]
        tgt = page + lo12
        in_got = self.img.got_range[0] <= tgt < self.img.got_range[1]
        if not in_got and how == "ldst" and acc == 8 and self.ours is not None and func_start is not None:
            # a pointer slot outside the .got range that our object loads through the GOT
            r = self.ours.ref_at(self.aliases.get(func, [func]), first.address - func_start)
            in_got = r is not None and r.type == R_LD64_GOT_LO12_NC
        if in_got:
            got = self.img.got_target(tgt)
            if not got and self.ours is not None and func_start is not None:
                # a GOT slot the binary only knows as a relative address (an unnamed local
                # function or object): use the name our object loads through the GOT
                r = self.ours.ref_at(self.aliases.get(func, [func]), first.address - func_start)
                if r is not None and r.type == R_LD64_GOT_LO12_NC and r.sym:
                    got = (r.sym, r.addend)
            if got:
                self.undefs.add(got[0])
                sec.reloc(rel_off, got[0], R_ADR_GOT_PAGE, got[1])
                sec.reloc(first.address - fn_addr, got[0], R_LD64_GOT_LO12_NC, got[1])
            return
        page_ref = None
        for how, cins, lo12, acc in cons:
            t = page + lo12
            fp = False
            if how == "ldst" and cins.operands and cins.operands[0].type == ARM64_OP_REG:
                rn = cins.reg_name(cins.operands[0].reg) or ""
                fp = rn[:1] in ("s", "d", "q", "h", "b") and not rn.startswith("sp")
            jt = None
            if how == "add":
                dst = cins.operands[0].reg
                jt = self._jump_table_shape(insns, insns.index(cins), dst, 0)
            ref = self._ref_data(t, how, acc, fp, func, jt,
                                 None if func_start is None else cins.address - func_start)
            if not ref:
                continue
            if page_ref is None:
                page_ref = ref
                sec.reloc(rel_off, ref[0], R_ADR_PREL_PG_HI21, ref[1])
            if how == "add":
                sec.reloc(cins.address - fn_addr, ref[0], R_ADD_ABS_LO12_NC, ref[1])
            else:
                sec.reloc(cins.address - fn_addr, ref[0], _LDST_BY_ACCESS.get(acc, R_LDST64_ABS_LO12_NC), ref[1])

    def _recon_data_abs64(self):
        for name, base, size in self.datas:
            sec = self.placed[name]
            base -= self.offs[name]
            for addr in range(base + self.offs[name], base + self.offs[name] + size, 8):
                ent = self.img.dyn_relocs.get(addr)
                if not ent:
                    continue
                rtype, nm, add = ent
                if rtype == R_ABS64 and nm:
                    if nm not in self.included:
                        self.undefs.add(nm)
                    sec.reloc(addr - base, nm, R_ABS64, add)
                elif rtype in (R_ABS64, 1027):  # RELATIVE: addend is the absolute VA
                    cov = self.img.sym_covering(add)
                    if cov:
                        tgt = cov[0].name
                        if tgt not in self.included:
                            self.undefs.add(tgt)
                        sec.reloc(addr - base, tgt, R_ABS64, cov[1])
                    else:
                        nm2 = f"lbl_{add + BASE:X}"
                        self.undefs.add(nm2)
                        sec.reloc(addr - base, nm2, R_ABS64, 0)

    def _clear_reloc_fields(self):
        """Zero the immediates a relocation will fill in, as in a real
        unlinked object (objdiff compares the raw instruction otherwise)."""
        import struct as _st
        for sec in self.o._secs:
            if not sec.name.startswith(".text"):
                continue
            for rel in sec.relocs:
                if rel.offset + 4 > len(sec.data):
                    continue
                w, = _st.unpack_from("<I", sec.data, rel.offset)
                t = rel.type
                if t in (R_CALL26, R_JUMP26):
                    w &= ~0x03FFFFFF
                elif t in (R_ADR_PREL_PG_HI21, R_ADR_GOT_PAGE):
                    w &= ~((0x3 << 29) | (0x7FFFF << 5))
                elif t in (R_ADD_ABS_LO12_NC, R_LDST8_ABS_LO12_NC, R_LDST16_ABS_LO12_NC,
                           R_LDST32_ABS_LO12_NC, R_LDST64_ABS_LO12_NC, R_LDST128_ABS_LO12_NC,
                           R_LD64_GOT_LO12_NC):
                    w &= ~(0xFFF << 10)
                else:
                    continue
                _st.pack_into("<I", sec.data, rel.offset, w)

    def write(self, path: str):
        self._clear_reloc_fields()
        # drop any empty section we speculatively created
        for s in list(self.o._secs):
            if s.type != E.SHT_NOBITS and not s.data and not s.relocs:
                self.o._secs.remove(s)
                del self.o._by_name[s.name]
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        Path(path).write_bytes(self.o.build())


def _parse_range(tok: str) -> tuple[str, int, int]:
    # ".text:0x1000:0x1234"  or  ".text:0x1000-0x1234"
    sec, _, rest = tok.partition(":")
    rest = rest.replace("-", ":")
    lo, _, hi = rest.partition(":")
    lo_i, hi_i = int(lo, 0), int(hi, 0)
    if lo_i >= BASE:
        lo_i -= BASE
    if hi_i >= BASE:
        hi_i -= BASE
    return sec, lo_i, hi_i


def _object_string_pools(path: str) -> dict[str, bytes]:
    """.rodata.str1.N section name -> contents, for a compiled object"""
    from elftools.elf.elffile import ELFFile
    out: dict[str, bytes] = {}
    with open(path, "rb") as fh:
        for sec in ELFFile(fh).iter_sections():
            if re.fullmatch(r"\.rodata\.str1\.\d+", sec.name):
                out[sec.name] = sec.data()
    return out


def _object_string_aligns(path: str) -> dict[bytes, int]:
    """string (with its NUL) -> alignment, for strings in a compiled object's .rodata.str1.N (N > 1)"""
    from elftools.elf.elffile import ELFFile
    out: dict[bytes, int] = {}
    with open(path, "rb") as fh:
        for sec in ELFFile(fh).iter_sections():
            m = re.fullmatch(r"\.rodata\.str1\.(\d+)", sec.name)
            if not m or int(m.group(1)) <= 1:
                continue
            al = int(m.group(1))
            data = sec.data()
            i = 0
            while i < len(data):
                j = data.find(b"\0", i)
                if j < 0:
                    break
                out.setdefault(data[i:j + 1], al)
                i = j + 1
                while i < len(data) and i % al and data[i] == 0:
                    i += 1
    return out


class OursRef:
    """One relocation in our compiled object, as seen from the function it sits in."""
    __slots__ = ("type", "sym", "symtype", "secname", "value", "size", "addend", "bind", "visibility")

    def __init__(self, type_, sym, symtype, secname, value, size, addend, bind, visibility):
        self.type, self.sym, self.symtype, self.secname = type_, sym, symtype, secname
        self.value, self.size, self.addend = value, size, addend
        self.bind = bind
        self.visibility = visibility


class OursObject:
    """Our compiled object for the unit: per-function relocations (keyed by offset from the
    function start), section contents, and the extents of the data they point at.  Used to
    rebuild references to data the binary's linker left unnamed the way clang emitted them."""

    def __init__(self, path: str):
        from elftools.elf.elffile import ELFFile
        from elftools.elf.relocation import RelocationSection
        self.funcs: dict[str, dict[int, OursRef]] = {}
        self.weak_functions: set[str] = set()
        self.interior_call_targets: set[str] = set()
        self.sec_data: dict[str, bytes] = {}
        self.sec_align: dict[str, int] = {}
        self.sec_nobits: dict[str, int] = {}
        self.local_objs: dict[str, list[tuple[int, int, str]]] = {}   # section -> (value, size, name)
        self.sec_addends: dict[str, set[int]] = {}                    # section -> referenced offsets
        with open(path, "rb") as fh:
            elf = ELFFile(fh)
            secs = list(elf.iter_sections())
            for s in secs:
                if s["sh_type"] == "SHT_NOBITS":
                    self.sec_nobits[s.name] = s["sh_size"]
                elif s["sh_flags"] & 2:   # SHF_ALLOC
                    self.sec_data[s.name] = s.data()
                self.sec_align[s.name] = s["sh_addralign"]
            symtab = elf.get_section_by_name(".symtab")
            if symtab is None:
                return
            syms = list(symtab.iter_symbols())
            funcs: dict[int, list[tuple[int, int, str]]] = {}
            for sym in syms:
                shndx = sym["st_shndx"]
                if not isinstance(shndx, int):
                    continue
                typ = sym["st_info"]["type"]
                if typ == "STT_FUNC":
                    if sym["st_info"]["bind"] == "STB_WEAK":
                        self.weak_functions.add(sym.name)
                    funcs.setdefault(shndx, []).append((sym["st_value"], sym["st_size"], sym.name))
                elif typ == "STT_OBJECT" and sym["st_info"]["bind"] == "STB_LOCAL":
                    self.local_objs.setdefault(secs[shndx].name, []).append(
                        (sym["st_value"], sym["st_size"], sym.name))
            for sec in secs:
                if not isinstance(sec, RelocationSection):
                    continue
                target = sec["sh_info"]
                for r in sec.iter_relocations():
                    sym = syms[r["r_info_sym"]]
                    shndx = sym["st_shndx"]
                    secname = secs[shndx].name if isinstance(shndx, int) and shndx else ""
                    typ = sym["st_info"]["type"]
                    add = r["r_addend"]
                    # data_value does not compare function-relocation addends.
                    # Do not let section guidance hide a call into a function's interior.
                    if r["r_info_type"] in (R_CALL26, R_JUMP26) and add != 0:
                        self.interior_call_targets.add(sym.name)
                    if typ == "STT_SECTION" and secname:
                        self.sec_addends.setdefault(secname, set()).add(add)
                    ref = OursRef(r["r_info_type"], sym.name if typ != "STT_SECTION" else secname,
                                  typ, secname, sym["st_value"], sym["st_size"], add, sym["st_info"]["bind"], sym["st_other"]["visibility"])
                    off = r["r_offset"]
                    for lo, size, fn in funcs.get(target, []):
                        if lo <= off < lo + max(size, 1):
                            self.funcs.setdefault(fn, {})[off - lo] = ref

    def ref_at(self, func_names, off: int) -> OursRef | None:
        for fn in func_names:
            r = self.funcs.get(fn, {}).get(off)
            if r is not None:
                return r
        return None

    def extent(self, secname: str, off: int) -> int:
        """How many bytes the data referenced at secname+off spans in our object."""
        for lo, size, _ in self.local_objs.get(secname, []):
            if lo <= off < lo + size:
                return lo + size - off
        end = len(self.sec_data.get(secname, b"")) or self.sec_nobits.get(secname, 0)
        nxt = [a for a in self.sec_addends.get(secname, ()) if a > off]
        for lo, _, _ in self.local_objs.get(secname, []):
            if lo > off:
                nxt.append(lo)
        return (min(nxt) if nxt else end) - off


def _object_refs(path: str) -> dict[str, set[str]]:
    """function name -> names its relocations reference, for a compiled object"""
    from elftools.elf.elffile import ELFFile
    from elftools.elf.relocation import RelocationSection
    out: dict[str, set[str]] = {}
    with open(path, "rb") as fh:
        elf = ELFFile(fh)
        symtab = elf.get_section_by_name(".symtab")
        funcs = {}
        for sym in symtab.iter_symbols():
            if sym["st_info"]["type"] == "STT_FUNC" and sym["st_shndx"] not in ("SHN_UNDEF", "SHN_ABS"):
                funcs.setdefault(sym["st_shndx"], []).append((sym["st_value"], sym["st_size"], sym.name))
        for sec in elf.iter_sections():
            if not isinstance(sec, RelocationSection):
                continue
            target = sec["sh_info"]
            for r in sec.iter_relocations():
                nm = symtab.get_symbol(r["r_info_sym"]).name
                off = r["r_offset"]
                for lo, size, fn in funcs.get(target, []):
                    if lo <= off < lo + max(size, 1):
                        out.setdefault(fn, set()).add(nm)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--elf", default="build/fury.elf")
    ap.add_argument("--map", default=None, help="function map TSV (names for local functions)")
    ap.add_argument("--syms", default=None, help="extra symbols (default: config/1.0.0/symbols.txt if present)")
    ap.add_argument("--range", action="append", default=[], metavar="SEC:LO:HI",
                    help="a section range this TU owns (repeatable)")
    ap.add_argument("--ours", default=None,
                    help="our compiled object for this unit: where a call could name either of "
                         "two aliases (C1/C2, D1/D2), use the one our code references")
    ap.add_argument("-q", "--quiet", action="store_true")
    ap.add_argument("symbols", nargs="*", help="explicit symbol names (alternative to --range)")
    a = ap.parse_args(argv)

    if not a.range and not a.symbols:
        ap.error("give at least one --range or symbol name")
    syms = a.syms
    if syms is None and Path("config/1.0.0/symbols.txt").is_file():
        syms = "config/1.0.0/symbols.txt"

    img = FuryImage(a.elf, extra_map=a.map, extra_syms=syms)
    c = Carver(img)
    if a.ours and Path(a.ours).is_file():
        c.ours_refs = _object_refs(a.ours)
        c.ours_str_align = _object_string_aligns(a.ours)
        c.ours_pools = _object_string_pools(a.ours)
        c.ours = OursObject(a.ours)
    if a.range:
        c.include_ranges([_parse_range(t) for t in a.range])
    if a.symbols:
        c.include(list(a.symbols))
    c.process()
    c.write(a.out)
    for wmsg in c.warnings:
        print(f"  warning: {wmsg}", file=sys.stderr)
    if not a.quiet:
        print(f"wrote {a.out}: {len(c.included)} symbols, "
              f"{sum(len(s.relocs) for s in c.o._secs)} relocs, "
              f"{len(c.undefs - c.included)} undefined refs")


if __name__ == "__main__":
    main()
