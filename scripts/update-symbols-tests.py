#!/usr/bin/env python3

"""
This script generate tests for symbols.
"""

import argparse
from dataclasses import dataclass
from enum import IntFlag
from pathlib import Path
from typing import Any, ClassVar, Iterator, Self

import jinja2

SCRIPT = Path(__file__)


@dataclass
class KeyCode:
    _evdev: str
    _xkb: str

    @property
    def c(self) -> str:
        return f"KEY_{self._evdev}"

    @property
    def xkb(self) -> str:
        return f"<{self._xkb}>"


class Keysym(str):
    @property
    def c(self) -> str:
        return f"XKB_KEY_{self}"

    @classmethod
    def parse(cls, raw: str | None) -> Self:
        if not raw:
            return cls("NoSymbol")
        else:
            return cls(raw)


NoSymbol = Keysym("NoSymbol")


class Modifier(IntFlag):
    NoModifier = 0
    Shift = 1 << 0
    Lock = 1 << 1
    Control = 1 << 2
    Mod1 = 1 << 3
    Mod2 = 1 << 4
    Mod3 = 1 << 5
    Mod4 = 1 << 6
    Mod5 = 1 << 7
    LevelThree = Mod5

    def __iter__(self) -> Iterator[Self]:
        for m in self.__class__:
            if m & self:
                yield m

    def __str__(self) -> str:
        return "+".join(self)


@dataclass
class Action:
    """
    SetMods or NoAction
    """

    mods: Modifier

    def __str__(self) -> str:
        if self.mods is Modifier.NoModifier:
            return "NoAction"
        else:
            return f"SetMods(mods={self.mods})"

    def __bool__(self) -> bool:
        return self.mods is not Modifier.NoModifier


@dataclass
class Level:
    keysyms: tuple[Keysym, ...]
    actions: tuple[Action, ...]

    @staticmethod
    def _c(default: str, xs: Iterator[Any]) -> str:
        match len(xs):
            case 0:
                return default
            case 1:
                return xs[0].c
            case _:
                return ", ".join(map(lambda x: x.c, xs))

    @staticmethod
    def _xkb(default: str, xs: Iterator[Any]) -> str:
        match len(xs):
            case 0:
                return default
            case 1:
                return str(xs[0])
            case _:
                return "{" + ", ".join(map(str, xs)) + "}"

    @classmethod
    def empty_symbols(cls, keysyms: tuple[Keysym, ...]) -> bool:
        return all(ks == NoSymbol for ks in keysyms)

    @property
    def keysyms_c(self) -> str:
        return self._c(NoSymbol.c, self.keysyms)

    @property
    def keysyms_xkb(self) -> str:
        return self._xkb(NoSymbol, self.keysyms)

    @classmethod
    def empty_actions(cls, actions: tuple[Action, ...]) -> bool:
        return not any(actions)

    @property
    def actions_xkb(self) -> str:
        return self._xkb("NoAction", self.actions)


@dataclass
class KeyEntry:
    levels: tuple[Level, ...]

    def __init__(self, *levels: Level):
        self.levels = levels

    @property
    def xkb(self) -> Iterator[str]:
        keysyms = tuple(l.keysyms for l in self.levels)
        has_keysyms = not any(Level.empty_symbols(s) for s in keysyms)
        actions = tuple(l.actions for l in self.levels)
        has_actions = not any(Level.empty_actions(a) for a in actions)
        if has_keysyms or not has_actions:
            yield "symbols=["
            yield ", ".join(l.keysyms_xkb for l in self.levels)
            yield "]"
        if has_actions:
            if has_keysyms:
                yield ", "
            yield "actions=["
            yield ", ".join(l.actions_xkb for l in self.levels)
            yield "]"


@dataclass
class TestEntry:
    key: KeyCode
    base: KeyEntry
    update: KeyEntry
    augment: KeyEntry
    override: KeyEntry
    # TODO: replace
    symbols_file: ClassVar[str] = "merge_modes"
    test_file: ClassVar[str] = "merge_modes.h"

    @classmethod
    def write_symbols(
        cls, root: Path, jinja_env: jinja2.Environment, tests: tuple[Self, ...]
    ) -> None:
        path = root / f"test/data/symbols/{cls.symbols_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    symbols_file=cls.symbols_file,
                    tests=tests,
                    script=SCRIPT.relative_to(root),
                )
            )

    @classmethod
    def write_c_tests(
        cls, root: Path, jinja_env: jinja2.Environment, tests: tuple[Self, ...]
    ) -> None:
        path = root / f"test/{cls.test_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    symbols_file=cls.symbols_file,
                    tests=tests,
                    script=SCRIPT.relative_to(root),
                )
            )


KEYSYMS_a = (Keysym("a"),)
KEYSYMS_A = (Keysym("A"),)
KEYSYMS_b = (Keysym("b"),)
KEYSYMS_B = (Keysym("B"),)
KEYSYMS___ = (NoSymbol, NoSymbol)
KEYSYMS_ab = (Keysym("a"), Keysym("b"))
KEYSYMS_AB = (Keysym("A"), Keysym("B"))
KEYSYMS_c_ = (Keysym("c"), NoSymbol)
KEYSYMS_C_ = (Keysym("C"), NoSymbol)
KEYSYMS__d = (NoSymbol, Keysym("d"))
KEYSYMS__D = (NoSymbol, Keysym("D"))
KEYSYMS_cd = (Keysym("c"), Keysym("d"))
KEYSYMS_CD = (Keysym("C"), Keysym("D"))
KEYSYMS_None = (NoSymbol,)


def KeysymsLevel(*keysyms: str | None) -> Level:
    return Level(tuple(map(Keysym.parse, keysyms)), ())


TESTS = (
    # Single keysyms -> single keysyms
    TestEntry(
        KeyCode("Q", "AD01"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        override=KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
    ),
    TestEntry(
        KeyCode("W", "AD02"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel("b"), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel("b"), KeysymsLevel(None)),
        override=KeyEntry(KeysymsLevel("b"), KeysymsLevel(None)),
    ),
    TestEntry(
        KeyCode("E", "AD03"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel("B")),
        augment=KeyEntry(KeysymsLevel(None), KeysymsLevel("B")),
        override=KeyEntry(KeysymsLevel(None), KeysymsLevel("B")),
    ),
    TestEntry(
        KeyCode("R", "AD04"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel("b"), KeysymsLevel("B")),
        augment=KeyEntry(KeysymsLevel("b"), KeysymsLevel("B")),
        override=KeyEntry(KeysymsLevel("b"), KeysymsLevel("B")),
    ),
    TestEntry(
        KeyCode("T", "AD05"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
    ),
    TestEntry(
        KeyCode("Y", "AD06"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel("b"), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("b"), KeysymsLevel("A")),
    ),
    TestEntry(
        KeyCode("U", "AD07"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel("B")),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("a"), KeysymsLevel("B")),
    ),
    TestEntry(
        KeyCode("I", "AD08"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel("b"), KeysymsLevel("B")),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("b"), KeysymsLevel("B")),
    ),
    # Single keysyms -> multiple keysyms
    TestEntry(
        KeyCode("A", "AC01"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None)),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None)),
    ),
    TestEntry(
        KeyCode("S", "AC02"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, None)),
        augment=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None)),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None)),
    ),
    TestEntry(
        KeyCode("D", "AC03"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel("C", None)),
        augment=KeyEntry(KeysymsLevel(None), KeysymsLevel("C", None)),
        override=KeyEntry(KeysymsLevel(None), KeysymsLevel("C", None)),
    ),
    TestEntry(
        KeyCode("F", "AC04"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("C", None)),
        augment=KeyEntry(KeysymsLevel(None), KeysymsLevel("C", None)),
        override=KeyEntry(KeysymsLevel(None), KeysymsLevel("C", None)),
    ),
    TestEntry(
        KeyCode("G", "AC05"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("C", None)),
        augment=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("C", None)),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("C", None)),
    ),
    TestEntry(
        KeyCode("H", "AC06"),
        KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
    ),
    TestEntry(
        KeyCode("J", "AC07"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("A")),
    ),
    TestEntry(
        KeyCode("K", "AC08"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("A")),
    ),
    TestEntry(
        KeyCode("L", "AC09"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel("C", None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("a"), KeysymsLevel("C", "")),
    ),
    TestEntry(
        KeyCode("SEMICOLON", "AC10"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("C", None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("a"), KeysymsLevel("C", None)),
    ),
    TestEntry(
        KeyCode("APOSTROPHE", "AC11"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("C", None)),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel("C", None)),
    ),
    TestEntry(
        KeyCode("BACKSLASH", "AC12"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
    ),
    # Multiple keysyms -> multiple keysyms
    TestEntry(
        KeyCode("Z", "AB01"),
        KeyEntry(KeysymsLevel(None, None), KeysymsLevel(None, None)),
        update=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("C", "D")),
        override=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("C", "D")),
    ),
    TestEntry(
        KeyCode("X", "AB02"),
        KeyEntry(KeysymsLevel(None, None), KeysymsLevel(None, None)),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, "D")),
        augment=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, "D")),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, "D")),
    ),
    TestEntry(
        KeyCode("C", "AB03"),
        KeyEntry(KeysymsLevel(None, None), KeysymsLevel("A", "B")),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("A", "B")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
    ),
    TestEntry(
        KeyCode("V", "AB04"),
        KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, "D")),
        augment=KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        override=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, "D")),
    ),
    TestEntry(
        KeyCode("B", "AB05"),
        KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel("a", "d"), KeysymsLevel("C", "B")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
    ),
    TestEntry(
        KeyCode("N", "AB06"),
        KeyEntry(KeysymsLevel("a", "b"), KeysymsLevel("A", "B")),
        update=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel("a", "b"), KeysymsLevel("A", "B")),
        override=KeyEntry(KeysymsLevel("a", "b"), KeysymsLevel("C", "D")),
    ),
    TestEntry(
        KeyCode("M", "AB07"),
        KeyEntry(KeysymsLevel("a", "b"), KeysymsLevel("A", "B")),
        update=KeyEntry(KeysymsLevel("c", None), KeysymsLevel(None, "D")),
        augment=KeyEntry(KeysymsLevel("a", "b"), KeysymsLevel("A", "B")),
        override=KeyEntry(KeysymsLevel("c", "b"), KeysymsLevel("A", "D")),
    ),
    # Multiple keysyms -> single keysyms
    TestEntry(
        KeyCode("GRAVE", "TLDE"),
        KeyEntry(KeysymsLevel(None, None), KeysymsLevel("A", "B")),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("A", "B")),
        override=KeyEntry(KeysymsLevel(None, None), KeysymsLevel("A", "B")),
    ),
    TestEntry(
        KeyCode("1", "AE01"),
        KeyEntry(KeysymsLevel(None, None), KeysymsLevel("A", "B")),
        update=KeyEntry(KeysymsLevel("c"), KeysymsLevel("C")),
        augment=KeyEntry(KeysymsLevel("c"), KeysymsLevel("A", "B")),
        override=KeyEntry(KeysymsLevel("c"), KeysymsLevel("C")),
    ),
    TestEntry(
        KeyCode("2", "AE02"),
        KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        update=KeyEntry(KeysymsLevel(None), KeysymsLevel(None)),
        augment=KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        override=KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
    ),
    TestEntry(
        KeyCode("3", "AE03"),
        KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        update=KeyEntry(KeysymsLevel("c"), KeysymsLevel("C")),
        augment=KeyEntry(KeysymsLevel("a", None), KeysymsLevel(None, "B")),
        override=KeyEntry(KeysymsLevel("c"), KeysymsLevel("C")),
    ),
    # Mix
    TestEntry(
        KeyCode("4", "AE04"),
        KeyEntry(KeysymsLevel("a")),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("C", "D")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C", "D")),
    ),
    TestEntry(
        KeyCode("5", "AE05"),
        KeyEntry(KeysymsLevel("a", "b")),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C")),
        augment=KeyEntry(KeysymsLevel("a", "b"), KeysymsLevel("C")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C")),
    ),
    TestEntry(
        KeyCode("6", "AE06"),
        KeyEntry(KeysymsLevel("a"), KeysymsLevel("A", "B")),
        update=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C")),
        augment=KeyEntry(KeysymsLevel("a"), KeysymsLevel("A", "B")),
        override=KeyEntry(KeysymsLevel("c", "d"), KeysymsLevel("C")),
    ),
)

if __name__ == "__main__":
    # Root of the project
    ROOT = Path(__file__).parent.parent

    # Parse commands
    parser = argparse.ArgumentParser(description="Generate symbols tests")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="Path to the root of the project (default: %(default)s)",
    )

    args = parser.parse_args()
    template_loader = jinja2.FileSystemLoader(args.root, encoding="utf-8")
    jinja_env = jinja2.Environment(
        loader=template_loader,
        keep_trailing_newline=True,
        trim_blocks=True,
        lstrip_blocks=True,
    )
    TestEntry.write_symbols(root=args.root, jinja_env=jinja_env, tests=TESTS)
    TestEntry.write_c_tests(root=args.root, jinja_env=jinja_env, tests=TESTS)
