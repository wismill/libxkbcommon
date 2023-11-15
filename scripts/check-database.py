#!/usr/bin/env python3

from dataclasses import dataclass
from enum import Enum, unique
from itertools import chain
from pathlib import Path
import sys
from typing import Iterable
import yaml


@unique
class Component(Enum):
    KEYCODES = "xkb_keycodes"
    COMPAT = "xkb_compatibility"
    SYMBOLS = "xkb_symbols"
    TYPES = "xkb_types"


@dataclass
class Include:
    name: str
    section: str | None
    path: Path


@dataclass
class RMLVO:
    rules: str
    model: str
    layout: str
    variant: str | None
    options: str | None


@dataclass
class Section:
    name: str
    default: bool
    registry: Iterable[RMLVO]
    includes: Iterable[Include]


@dataclass
class ComponentFile:
    name: str
    canonical: bool
    sections: Iterable[Section]


x = yaml.safe_load(sys.stdin)


def add_rmlvos_to_all_deps(files, rmlvos, includes):
    for include in includes:
        file_conf = files.get(include.get("path"))
        assert file_conf, include.get("path")
        section_name = include.get("section")
        for section in file_conf.get("sections"):
            if section.get("name") == section_name or (
                section.get("default", False) and not section_name
            ):
                registry = section.get("registry", [])
                if not registry:
                    section["registry"] = list(map(dict, rmlvos))
                else:
                    section["registry"] = list(
                        chain(
                            registry,
                            (
                                dict(r)
                                for r in rmlvos
                                if all(r["rules"] != r2["rules"] for r2 in registry)
                            ),
                        )
                    )
                add_rmlvos_to_all_deps(files, rmlvos, section.get("includes", ()))


def add_dep(files, dep, includes):
    for include in includes:
        file_conf = files.get(include.get("path"))
        assert file_conf, include.get("path")
        section_name = include.get("section")
        for section in file_conf.get("sections"):
            if section.get("name") == section_name or (
                section.get("default", False) and not section_name
            ):
                deps = section.get("included by", [])
                deps.append(dict(dep))
                section["included by"] = deps


for component in Component:
    files = x.get(component.value)
    for path, file_conf in files.items():
        if file_conf.get("canonical") and not file_conf.get("error"):
            for section in file_conf.get("sections"):
                if includes := section.get("includes"):
                    dep = {
                        "name": file_conf.get("name"),
                        "section": section.get("name"),
                        "path": path,
                    }
                    add_dep(files, dep, includes)
                    if registry := section.get("registry"):
                        add_rmlvos_to_all_deps(files, registry, includes)

# yaml.safe_dump(x, sys.stdout, encoding="utf-8", sort_keys=False, default_style="\"")
yaml.safe_dump(x, sys.stdout, encoding="utf-8", sort_keys=False)
