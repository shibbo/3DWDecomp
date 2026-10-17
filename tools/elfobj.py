#!/usr/bin/env python3
"""Minimal ELF64 little-endian relocatable-object writer.

Just enough to emit the target `.o` files the splitter produces.  pyelftools is
read-only, so the writer is by hand.  Usage:

    o = ElfObject()
    text = o.section(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXEC, align=4)
    text.data += code_bytes
    o.define("Foo", text, off=0, size=0x20, typ=STT_FUNC)
    o.undef("Bar")
    text.reloc(off=0x10, sym="Bar", type=R_AARCH64_CALL26)
    open("foo.o", "wb").write(o.build())
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field

SHT_NULL, SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS = 0, 1, 2, 3, 4, 8
SHF_WRITE, SHF_ALLOC, SHF_EXEC = 0x1, 0x2, 0x4
STB_LOCAL, STB_GLOBAL, STB_WEAK = 0, 1, 2
STT_NOTYPE, STT_OBJECT, STT_FUNC, STT_SECTION = 0, 1, 2, 3
SHN_UNDEF, SHN_ABS = 0, 0xFFF1
EM_AARCH64, ET_REL = 183, 1


@dataclass
class _Rel:
    offset: int
    sym: str
    type: int
    addend: int = 0


@dataclass
class _Sym:
    name: str
    shndx: int
    value: int
    size: int
    bind: int
    typ: int
    visibility: int = 0


class Section:
    def __init__(self, name, type_, flags=0, align=1, entsize=0):
        self.name = name
        self.type = type_
        self.flags = flags
        self.align = align
        self.entsize = entsize
        self.data = bytearray()
        self.nobits_size = 0
        self.relocs: list[_Rel] = []
        self.idx = 0

    def reloc(self, off, sym, type, addend=0):
        self.relocs.append(_Rel(off, sym, type, addend))


class ElfObject:
    def __init__(self):
        self._secs: list[Section] = []
        self._by_name: dict[str, Section] = {}
        self._defs: list[_Sym] = []
        self._undefs: list[str] = []

    def section(self, name, type_, flags=0, align=1, entsize=0) -> Section:
        s = self._by_name.get(name)
        if s is None:
            s = Section(name, type_, flags, align, entsize)
            self._secs.append(s)
            self._by_name[name] = s
        return s

    def define(self, name, sec: Section, off, size, typ=STT_FUNC, bind=STB_GLOBAL, visibility=0):
        # store the Section object in `shndx`; resolved to its index at build()
        self._defs.append(_Sym(name, sec, off, size, bind, typ, visibility))

    def undef(self, name):
        if name not in self._undefs:
            self._undefs.append(name)

    # ------------------------------------------------------------------
    def build(self) -> bytes:
        content = list(self._secs)
        rela_for = [s for s in content if s.relocs]

        # section table order: NULL, content..., .symtab, .strtab, .rela.*, .shstrtab
        shs: list[Section] = [Section("", SHT_NULL)]
        shs += content
        symtab = Section(".symtab", SHT_SYMTAB, align=8, entsize=24)
        strtab = Section(".strtab", SHT_STRTAB, align=1)
        shs += [symtab, strtab]
        relas: list[tuple[Section, Section]] = []
        for s in rela_for:
            r = Section(".rela" + s.name, SHT_RELA, align=8, entsize=24)
            shs.append(r)
            relas.append((r, s))
        shstr = Section(".shstrtab", SHT_STRTAB, align=1)
        shs.append(shstr)
        for i, s in enumerate(shs):
            s.idx = i

        # ---- .strtab + symbol table ----
        strb = bytearray(b"\0")
        def sadd(t: str) -> int:
            if not t:
                return 0
            i = len(strb); strb.extend(t.encode() + b"\0"); return i

        # order: index0 null, section syms (local), other locals, then globals
        rows: list[tuple[int, int, int, int, int]] = []  # nameoff, info, shndx, value, size
        symidx: dict[str, int] = {}

        rows.append((0, 0, 0, 0, 0, 0))
        secsym_idx: dict[str, int] = {}
        for s in content:
            secsym_idx[s.name] = len(rows)
            rows.append((sadd(s.name), (STB_LOCAL << 4) | STT_SECTION, 0, s.idx, 0, 0))

        local_defs = [d for d in self._defs if d.bind == STB_LOCAL]
        global_defs = [d for d in self._defs if d.bind != STB_LOCAL]
        for d in local_defs:
            symidx[d.name] = len(rows)
            rows.append((sadd(d.name), (STB_LOCAL << 4) | d.typ, d.visibility, d.shndx.idx, d.value, d.size))
        first_global = len(rows)
        for d in global_defs:
            symidx[d.name] = len(rows)
            rows.append((sadd(d.name), (d.bind << 4) | d.typ, d.visibility, d.shndx.idx, d.value, d.size))
        for n in self._undefs:
            if n in symidx:
                continue
            symidx[n] = len(rows)
            rows.append((sadd(n), (STB_GLOBAL << 4) | STT_NOTYPE, 0, SHN_UNDEF, 0, 0))

        symtab.data = bytearray()
        for nameoff, info, visibility, shndx, value, size in rows:
            symtab.data += struct.pack("<IBBHQQ", nameoff, info, visibility, shndx, value, size)
        strtab.data = strb

        # ---- rela sections ----
        for r_sec, tgt in relas:
            b = bytearray()
            for rel in tgt.relocs:
                si = symidx.get(rel.sym)
                if si is None:
                    si = secsym_idx.get(rel.sym, 0)
                b += struct.pack("<QQq", rel.offset, (si << 32) | rel.type, rel.addend)
            r_sec.data = b
            r_sec._link = symtab.idx
            r_sec._info = tgt.idx

        # ---- .shstrtab ----
        shname: dict[int, int] = {}
        shb = bytearray(b"\0")
        for s in shs:
            if s is shstr:
                continue
            shname[s.idx] = len(shb); shb.extend(s.name.encode() + b"\0")
        shname[shstr.idx] = len(shb); shb.extend(b".shstrtab\0")
        shstr.data = shb

        # ---- file layout ----
        ehsize, shentsize = 64, 64
        out = bytearray(ehsize)
        offs: dict[int, int] = {}
        for s in shs:
            if s.type in (SHT_NULL, SHT_NOBITS):
                offs[s.idx] = 0
                continue
            while len(out) % max(s.align, 1):
                out.append(0)
            offs[s.idx] = len(out)
            out.extend(s.data)
        while len(out) % 8:
            out.append(0)
        sht_off = len(out)

        for s in shs:
            link = info = 0
            flags = s.flags
            size = s.nobits_size if s.type == SHT_NOBITS else len(s.data)
            if s.type == SHT_SYMTAB:
                link, info = strtab.idx, first_global
            elif s.type == SHT_RELA:
                link, info, flags = s._link, s._info, 0
            out += struct.pack("<IIQQQQIIQQ", shname[s.idx], s.type, flags, 0,
                               offs[s.idx], size, link, info, max(s.align, 1), s.entsize)

        out[0:ehsize] = struct.pack(
            "<16sHHIQQQIHHHHHH",
            b"\x7fELF" + bytes([2, 1, 1, 0]) + b"\0" * 8,
            ET_REL, EM_AARCH64, 1, 0, 0, sht_off, 0,
            ehsize, 0, 0, shentsize, len(shs), shstr.idx,
        )
        return bytes(out)
