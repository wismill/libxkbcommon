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
    SetGroup or NoAction
    """

    group: int

    def __str__(self) -> str:
        if self.group > 0:
            return f"SetGroup(group={self.group})"
        else:
            return "NoAction"

    def __bool__(self) -> bool:
        return bool(self.group)


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

    @classmethod
    def Keysyms(cls, *keysyms: str | None) -> Self:
        return cls(tuple(map(Keysym.parse, keysyms)), ())


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
                    tests_groups=tests,
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
                    tests_groups=tests,
                    script=SCRIPT.relative_to(root),
                )
            )


@dataclass
class TestGroup:
    name: str
    tests: tuple[TestEntry, ...]


TESTS_KEYSYMS_ONLY = TestGroup(
    "keysyms_only",
    (
        # Single keysyms -> single keysyms
        TestEntry(
            KeyCode("Q", "AD01"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            override=KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
        ),
        TestEntry(
            KeyCode("W", "AD02"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms("b"), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms("b"), Level.Keysyms(None)),
            override=KeyEntry(Level.Keysyms("b"), Level.Keysyms(None)),
        ),
        TestEntry(
            KeyCode("E", "AD03"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms("B")),
            augment=KeyEntry(Level.Keysyms(None), Level.Keysyms("B")),
            override=KeyEntry(Level.Keysyms(None), Level.Keysyms("B")),
        ),
        TestEntry(
            KeyCode("R", "AD04"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms("b"), Level.Keysyms("B")),
            augment=KeyEntry(Level.Keysyms("b"), Level.Keysyms("B")),
            override=KeyEntry(Level.Keysyms("b"), Level.Keysyms("B")),
        ),
        TestEntry(
            KeyCode("T", "AD05"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
        ),
        TestEntry(
            KeyCode("Y", "AD06"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms("b"), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("b"), Level.Keysyms("A")),
        ),
        TestEntry(
            KeyCode("U", "AD07"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms("B")),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("a"), Level.Keysyms("B")),
        ),
        TestEntry(
            KeyCode("I", "AD08"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms("b"), Level.Keysyms("B")),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("b"), Level.Keysyms("B")),
        ),
        # Single keysyms -> multiple keysyms
        TestEntry(
            KeyCode("A", "AC01"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None)),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None)),
        ),
        TestEntry(
            KeyCode("S", "AC02"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, None)),
            augment=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None)),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None)),
        ),
        TestEntry(
            KeyCode("D", "AC03"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms("C", None)),
            augment=KeyEntry(Level.Keysyms(None), Level.Keysyms("C", None)),
            override=KeyEntry(Level.Keysyms(None), Level.Keysyms("C", None)),
        ),
        TestEntry(
            KeyCode("F", "AC04"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("C", None)),
            augment=KeyEntry(Level.Keysyms(None), Level.Keysyms("C", None)),
            override=KeyEntry(Level.Keysyms(None), Level.Keysyms("C", None)),
        ),
        TestEntry(
            KeyCode("G", "AC05"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("C", None)),
            augment=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("C", None)),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("C", None)),
        ),
        TestEntry(
            KeyCode("H", "AC06"),
            KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
        ),
        TestEntry(
            KeyCode("J", "AC07"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("A")),
        ),
        TestEntry(
            KeyCode("K", "AC08"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("A")),
        ),
        TestEntry(
            KeyCode("L", "AC09"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms("C", None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("a"), Level.Keysyms("C", "")),
        ),
        TestEntry(
            KeyCode("SEMICOLON", "AC10"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("C", None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("a"), Level.Keysyms("C", None)),
        ),
        TestEntry(
            KeyCode("APOSTROPHE", "AC11"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("C", None)),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms("C", None)),
        ),
        TestEntry(
            KeyCode("BACKSLASH", "AC12"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
        ),
        # Multiple keysyms -> multiple keysyms
        TestEntry(
            KeyCode("Z", "AB01"),
            KeyEntry(Level.Keysyms(None, None), Level.Keysyms(None, None)),
            update=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("C", "D")),
            override=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("C", "D")),
        ),
        TestEntry(
            KeyCode("X", "AB02"),
            KeyEntry(Level.Keysyms(None, None), Level.Keysyms(None, None)),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, "D")),
            augment=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, "D")),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, "D")),
        ),
        TestEntry(
            KeyCode("C", "AB03"),
            KeyEntry(Level.Keysyms(None, None), Level.Keysyms("A", "B")),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("A", "B")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
        ),
        TestEntry(
            KeyCode("V", "AB04"),
            KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, "D")),
            augment=KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            override=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, "D")),
        ),
        TestEntry(
            KeyCode("B", "AB05"),
            KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms("a", "d"), Level.Keysyms("C", "B")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
        ),
        TestEntry(
            KeyCode("N", "AB06"),
            KeyEntry(Level.Keysyms("a", "b"), Level.Keysyms("A", "B")),
            update=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms("a", "b"), Level.Keysyms("A", "B")),
            override=KeyEntry(Level.Keysyms("a", "b"), Level.Keysyms("C", "D")),
        ),
        TestEntry(
            KeyCode("M", "AB07"),
            KeyEntry(Level.Keysyms("a", "b"), Level.Keysyms("A", "B")),
            update=KeyEntry(Level.Keysyms("c", None), Level.Keysyms(None, "D")),
            augment=KeyEntry(Level.Keysyms("a", "b"), Level.Keysyms("A", "B")),
            override=KeyEntry(Level.Keysyms("c", "b"), Level.Keysyms("A", "D")),
        ),
        # Multiple keysyms -> single keysyms
        TestEntry(
            KeyCode("GRAVE", "TLDE"),
            KeyEntry(Level.Keysyms(None, None), Level.Keysyms("A", "B")),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("A", "B")),
            override=KeyEntry(Level.Keysyms(None, None), Level.Keysyms("A", "B")),
        ),
        TestEntry(
            KeyCode("1", "AE01"),
            KeyEntry(Level.Keysyms(None, None), Level.Keysyms("A", "B")),
            update=KeyEntry(Level.Keysyms("c"), Level.Keysyms("C")),
            augment=KeyEntry(Level.Keysyms("c"), Level.Keysyms("A", "B")),
            override=KeyEntry(Level.Keysyms("c"), Level.Keysyms("C")),
        ),
        TestEntry(
            KeyCode("2", "AE02"),
            KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            update=KeyEntry(Level.Keysyms(None), Level.Keysyms(None)),
            augment=KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            override=KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
        ),
        TestEntry(
            KeyCode("3", "AE03"),
            KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            update=KeyEntry(Level.Keysyms("c"), Level.Keysyms("C")),
            augment=KeyEntry(Level.Keysyms("a", None), Level.Keysyms(None, "B")),
            override=KeyEntry(Level.Keysyms("c"), Level.Keysyms("C")),
        ),
        # Mix
        TestEntry(
            KeyCode("4", "AE04"),
            KeyEntry(Level.Keysyms("a")),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("C", "D")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C", "D")),
        ),
        TestEntry(
            KeyCode("5", "AE05"),
            KeyEntry(Level.Keysyms("a", "b")),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C")),
            augment=KeyEntry(Level.Keysyms("a", "b"), Level.Keysyms("C")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C")),
        ),
        TestEntry(
            KeyCode("6", "AE06"),
            KeyEntry(Level.Keysyms("a"), Level.Keysyms("A", "B")),
            update=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C")),
            augment=KeyEntry(Level.Keysyms("a"), Level.Keysyms("A", "B")),
            override=KeyEntry(Level.Keysyms("c", "d"), Level.Keysyms("C")),
        ),
    ),
)

TESTS = (TESTS_KEYSYMS_ONLY,)

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
