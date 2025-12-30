#!/usr/bin/env python3
"""`astrac c-import` implementation for v0.

Parses a C header with Clang's JSON AST dump and emits an Astralis foreign
module containing:

- `foreign define` function prototypes
- `foreign declare` globals
- `foreign type` structs, unions, enums, and typedef aliases

Out-of-scope constructs are rejected with actionable messages (e.g. variadic
functions) to avoid generating broken bindings.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence


@dataclass
class FunctionParam:
    name: str
    type: str


@dataclass
class FunctionDecl:
    name: str
    return_type: str
    params: List[FunctionParam]
    variadic: bool = False


@dataclass
class StructField:
    name: str
    type: str


@dataclass
class StructDecl:
    name: str
    fields: List[StructField]


@dataclass
class EnumConst:
    name: str
    value: str


@dataclass
class EnumDecl:
    name: str
    underlying: str
    values: List[EnumConst]


@dataclass
class TypedefDecl:
    name: str
    target: str


@dataclass
class GlobalDecl:
    name: str
    type: str


class ClangAstImporter:
    def __init__(self, header: Path, includes: Sequence[str], defines: Sequence[str], target: Optional[str]):
        self.header = header.resolve()
        self.includes = list(includes)
        self.defines = list(defines)
        self.target = target

    def parse(self) -> Dict[str, List]:
        tree = self._load_ast()
        functions: List[FunctionDecl] = []
        structs: List[StructDecl] = []
        unions: List[StructDecl] = []
        enums: List[EnumDecl] = []
        typedefs: List[TypedefDecl] = []
        globals: List[GlobalDecl] = []

        for node in self._walk(tree):
            if node.get("isImplicit"):
                continue

            kind = node.get("kind")
            if kind == "FunctionDecl" and self._from_header(node):
                func = self._parse_function(node)
                if func.variadic:
                    raise ValueError(f"Variadic function not supported in v0: {func.name}")
                functions.append(func)
            elif kind == "VarDecl" and self._from_header(node):
                globals.append(self._parse_global(node))
            elif kind == "RecordDecl" and node.get("completeDefinition") and self._from_header(node):
                if not node.get("name"):
                    continue
                if node.get("tagUsed") == "union":
                    unions.append(self._parse_struct(node))
                else:
                    structs.append(self._parse_struct(node))
            elif kind == "EnumDecl" and self._from_header(node) and node.get("name"):
                enums.append(self._parse_enum(node))
            elif kind == "TypedefDecl" and self._from_header(node):
                typedefs.append(self._parse_typedef(node))

        return {
            "functions": sorted(functions, key=lambda f: f.name),
            "structs": sorted(structs, key=lambda s: s.name),
            "unions": sorted(unions, key=lambda u: u.name),
            "enums": sorted(enums, key=lambda e: e.name),
            "typedefs": sorted(typedefs, key=lambda t: t.name),
            "globals": sorted(globals, key=lambda g: g.name),
        }

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
            included = loc.get("includedFrom", {})
            if included.get("file"):
                return Path(included["file"]).resolve() == self.header
            return "offset" in loc or "line" in loc
        return Path(file).resolve() == self.header

    def _parse_function(self, node: dict) -> FunctionDecl:
        function_type = node.get("type", {})
        qual_type = function_type.get("qualType", "")
        return_type = self._map_ctype(qual_type.split("(")[0].strip())

        params: List[FunctionParam] = []
        for idx, child in enumerate(node.get("inner") or []):
            if child.get("kind") != "ParmVarDecl":
                continue
            name = child.get("name") or f"param{idx + 1}"
            param_type = self._map_ctype(child["type"]["qualType"])
            params.append(FunctionParam(name=name, type=param_type))

        variadic = bool(function_type.get("extInfo", {}).get("variadic"))
        return FunctionDecl(name=node["name"], return_type=return_type, params=params, variadic=variadic)

    def _parse_struct(self, node: dict) -> StructDecl:
        fields: List[StructField] = []
        for child in node.get("inner") or []:
            if child.get("kind") != "FieldDecl":
                continue
            field_type = self._map_ctype(child["type"]["qualType"])
            fields.append(StructField(name=child.get("name", "field"), type=field_type))
        return StructDecl(name=node["name"], fields=fields)

    def _parse_enum(self, node: dict) -> EnumDecl:
        underlying = "c.i32"
        integer_type = node.get("integerType") or {}
        if "qualType" in integer_type:
            underlying = self._map_ctype(integer_type["qualType"])

        values: List[EnumConst] = []
        current_value = -1
        for child in node.get("inner") or []:
            if child.get("kind") != "EnumConstantDecl":
                continue
            value_expr = child.get("initExpr") or child.get("value")

            if isinstance(value_expr, dict) and "value" in value_expr:
                current_value = int(value_expr["value"])
            elif value_expr is not None:
                current_value = int(value_expr)
            else:
                current_value += 1

            values.append(EnumConst(name=child.get("name", ""), value=str(current_value)))
        return EnumDecl(name=node["name"], underlying=underlying, values=values)

    def _parse_typedef(self, node: dict) -> TypedefDecl:
        name = node.get("name", "")
        target_type = node.get("type", {}).get("desugaredQualType") or node.get("type", {}).get("qualType", "")
        target = self._map_ctype(target_type)
        return TypedefDecl(name=name, target=target)

    def _parse_global(self, node: dict) -> GlobalDecl:
        return GlobalDecl(name=node.get("name", ""), type=self._map_ctype(node.get("type", {}).get("qualType", "")))

    def _map_ctype(self, qual: str) -> str:
        qual = qual.strip()
        qual = re.sub(r"\b(const|volatile|restrict)\s+", "", qual)

        # Arrays decay to pointers for v0.
        array_match = re.match(r"(.+)\s*\[(?:[^\]]*)\]$", qual)
        if array_match:
            return f"c.ptr<{self._map_ctype(array_match.group(1))}>"

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
        if lower in {"long long", "long long int", "signed long long", "signed long long int"}:
            return "c.i64"
        if lower in {"unsigned long long", "unsigned long long int"}:
            return "c.u64"
        if lower in {"char", "signed char"}:
            return "c.i8"
        if lower == "unsigned char":
            return "c.u8"
        if lower in {"bool", "_bool", "_Bool".lower()}:
            return "c.bool"
        if lower == "float":
            return "c.f32"
        if lower == "double":
            return "c.f64"
        if lower == "size_t":
            return "c.usize"
        if lower == "ssize_t":
            return "c.isize"
        if lower == "ptrdiff_t":
            return "c.isize"
        if lower.startswith("struct "):
            return lower.split("struct ", 1)[1]
        if lower.startswith("union "):
            return lower.split("union ", 1)[1]
        if lower.startswith("enum "):
            return lower.split("enum ", 1)[1]

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

    if decls.get("unions"):
        for union in decls["unions"]:
            lines.append(f"  foreign type {union.name} layout c union:")
            for field in union.fields:
                lines.append(f"    field {field.name}: {field.type}")
            lines.append("")

    if decls.get("enums"):
        for enum in decls["enums"]:
            lines.append(f"  foreign type {enum.name} as {enum.underlying} enum:")
            for value in enum.values:
                lines.append(f"    case {value.name} = {value.value}")
            lines.append("")

    if decls.get("typedefs"):
        for typedef in decls["typedefs"]:
            lines.append(f"  foreign type {typedef.name} as {typedef.target}")

        if decls["typedefs"]:
            lines.append("")

    if decls.get("globals"):
        for glob in decls["globals"]:
            lines.append(f"  foreign declare {glob.name}: {glob.type}")
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
    try:
        decls = importer.parse()
    except subprocess.CalledProcessError as exc:
        raise SystemExit(f"clang invocation failed: {exc}") from exc
    except ValueError as exc:
        raise SystemExit(str(exc)) from exc

    rendered = render_module(module_name, args.link, decls, source=args.header)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(rendered)
    print(f"Wrote {args.output} (module {module_name})")


if __name__ == "__main__":
    main()
