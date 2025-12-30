#!/usr/bin/env python3
"""Minimal `astrac c-import` prototype.

Parses a C header with Clang's JSON AST dump and emits a skeleton Astralis
foreign module with function and struct declarations. Designed to keep the
bindings/ directory populated with working examples while the full toolchain
is developed.
"""

from __future__ import annotations

import argparse
import json
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional


@dataclass
class FunctionParam:
    name: str
    type: str


@dataclass
class FunctionDecl:
    name: str
    return_type: str
    params: List[FunctionParam]


@dataclass
class StructField:
    name: str
    type: str


@dataclass
class StructDecl:
    name: str
    fields: List[StructField]


class ClangAstImporter:
    def __init__(self, header: Path, includes: List[str], defines: List[str], target: Optional[str]):
        self.header = header
        self.includes = includes
        self.defines = defines
        self.target = target

    def parse(self) -> Dict[str, List]:
        tree = self._load_ast()
        functions: List[FunctionDecl] = []
        structs: List[StructDecl] = []

        for node in self._walk(tree):
            if node.get("isImplicit"):
                continue

            if node.get("kind") == "FunctionDecl" and self._from_header(node):
                functions.append(self._parse_function(node))
            elif node.get("kind") == "RecordDecl" and node.get("completeDefinition"):
                if self._from_header(node) and node.get("name"):
                    structs.append(self._parse_struct(node))

        return {"functions": functions, "structs": structs}

    def _load_ast(self) -> dict:
        cmd = [
            "clang",
            "-Xclang",
            "-ast-dump=json",
            "-fsyntax-only",
            "-x",
            "c",
            "-std=c11",
        ]

        if self.target:
            cmd += ["--target", self.target]

        for inc in self.includes:
            cmd += ["-I", inc]
        for define in self.defines:
            cmd += ["-D", define]

        cmd.append(str(self.header))

        output = subprocess.check_output(cmd)
        return json.loads(output)

    def _walk(self, node: dict) -> Iterable[dict]:
        yield node
        for child in node.get("inner") or []:
            yield from self._walk(child)

    def _from_header(self, node: dict) -> bool:
        loc = node.get("loc", {})
        file = loc.get("file")
        if not file:
            # Some Clang AST nodes omit the file for simple prototypes; when
            # running against a single header, treat them as local.
            return True
        return Path(file) == self.header

    def _parse_function(self, node: dict) -> FunctionDecl:
        qual_type = node["type"]["qualType"]
        return_type = self._map_ctype(qual_type.split("(")[0].strip())

        params: List[FunctionParam] = []
        for idx, child in enumerate(node.get("inner") or []):
            if child.get("kind") != "ParmVarDecl":
                continue
            name = child.get("name") or f"param{idx + 1}"
            param_type = self._map_ctype(child["type"]["qualType"])
            params.append(FunctionParam(name=name, type=param_type))

        return FunctionDecl(name=node["name"], return_type=return_type, params=params)

    def _parse_struct(self, node: dict) -> StructDecl:
        fields: List[StructField] = []
        for child in node.get("inner") or []:
            if child.get("kind") != "FieldDecl":
                continue
            field_type = self._map_ctype(child["type"]["qualType"])
            fields.append(StructField(name=child.get("name", "field"), type=field_type))
        return StructDecl(name=node["name"], fields=fields)

    def _map_ctype(self, qual: str) -> str:
        qual = qual.replace("const ", "").replace("volatile ", "").strip()

        if qual.endswith("*"):
            base = qual[:-1].strip()
            if base == "char":
                return "c.cstring"
            if base == "const char":
                return "c.const_cstring"
            return f"c.ptr<{self._map_ctype(base)}>"

        lower = qual.lower()
        if lower == "void":
            return "c.void"
        if lower in {"int", "signed int"}:
            return "c.i32"
        if lower == "unsigned int":
            return "c.u32"
        if lower in {"short", "short int"}:
            return "c.i16"
        if lower in {"unsigned short", "unsigned short int"}:
            return "c.u16"
        if lower in {"long", "long int"}:
            return "c.i64"
        if lower in {"long unsigned int", "unsigned long", "unsigned long int"}:
            return "c.u64"
        if lower in {"char", "signed char"}:
            return "c.i8"
        if lower == "unsigned char":
            return "c.u8"
        if lower == "float":
            return "c.f32"
        if lower == "double":
            return "c.f64"
        if lower == "size_t":
            return "c.usize"
        if lower.startswith("struct "):
            return lower.split("struct ", 1)[1]

        return qual


def render_module(module: str, links: List[str], decls: Dict[str, List], source: Path) -> str:
    lines = [
        "# Auto-generated by tools/astrac_c_import.py",
        f"# Source: {source}",
    ]

    link_clause = ", ".join(f'"{lib}"' for lib in links) if links else ""
    module_header = f"foreign module {module}"
    if link_clause:
        module_header += f" links {link_clause}"
    module_header += ":"
    lines.append(module_header)

    if decls.get("structs"):
        for struct in decls["structs"]:
            lines.append(f"  foreign type {struct.name} layout c:")
            for field in struct.fields:
                lines.append(f"    field {field.name}: {field.type}")
            lines.append("")

    for func in decls.get("functions", []):
        params = ", ".join(f"{p.name}: {p.type}" for p in func.params)
        lines.append(f"  foreign define {func.name}({params}) -> {func.return_type}")

    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate Astralis bindings from a C header using Clang.")
    parser.add_argument("header", type=Path, help="C header to import")
    parser.add_argument("--link", "-l", action="append", default=[], help="Libraries to link against")
    parser.add_argument("--include", "-I", action="append", default=[], help="Additional include paths")
    parser.add_argument("--define", "-D", action="append", default=[], help="Macro definitions for Clang")
    parser.add_argument("--target", help="Optional Clang target triple")
    parser.add_argument("--module", help="Override module name (default: header stem)")
    parser.add_argument("--output", "-o", type=Path, required=True, help="Output Astralis bindings file")

    args = parser.parse_args()
    module_name = args.module or args.header.stem.replace("-", "_")

    importer = ClangAstImporter(args.header, args.include, args.define, args.target)
    decls = importer.parse()

    rendered = render_module(module_name, args.link, decls, source=args.header)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(rendered)
    print(f"Wrote {args.output} (module {module_name})")


if __name__ == "__main__":
    main()
