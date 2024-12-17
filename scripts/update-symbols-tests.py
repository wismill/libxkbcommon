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
        return self._c(Keysym("NoSymbol").c, self.keysyms)

    @property
    def keysyms_xkb(self) -> str:
        return self._xkb("NoSymbol", self.keysyms)

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
    conflict: KeyEntry
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

KEY_SIMPLE___ = KeyEntry(Level(KEYSYMS_None, ()), Level(KEYSYMS_None, ()))
KEY_SIMPLE_aA = KeyEntry(Level(KEYSYMS_a, ()), Level(KEYSYMS_A, ()))
KEY_SIMPLE_bB = KeyEntry(Level(KEYSYMS_b, ()), Level(KEYSYMS_B, ()))
KEY_SIMPLE__B = KeyEntry(Level(KEYSYMS_None, ()), Level(KEYSYMS_B, ()))
KEY_SIMPLE_b_ = KeyEntry(Level(KEYSYMS_b, ()), Level(KEYSYMS_None, ()))
KEY_SIMPLE_bA = KeyEntry(Level(KEYSYMS_b, ()), Level(KEYSYMS_A, ()))
KEY_SIMPLE_aB = KeyEntry(Level(KEYSYMS_a, ()), Level(KEYSYMS_B, ()))

KEY_MULTIPLE_AB = KeyEntry(Level(KEYSYMS_ab, ()), Level(KEYSYMS_AB, ()))
KEY_MULTIPLE_CD = KeyEntry(Level(KEYSYMS_cd, ()), Level(KEYSYMS_CD, ()))
KEY_MULTIPLE_cCD = KeyEntry(Level(KEYSYMS_cd, ()), Level(KEYSYMS_CD, ()))

KEY_MIXED_c__ = KeyEntry(Level(KEYSYMS_c_, ()), Level(KEYSYMS_None, ()))
KEY_MIXED_c___ = KeyEntry(Level(KEYSYMS_c_, ()), Level(KEYSYMS___, ()))
KEY_MIXED__C_ = KeyEntry(Level(KEYSYMS_None, ()), Level(KEYSYMS_C_, ()))
KEY_MIXED___C_ = KeyEntry(Level(KEYSYMS___, ()), Level(KEYSYMS_C_, ()))
KEY_MIXED_cC__ = KeyEntry(Level(KEYSYMS_c_, ()), Level(KEYSYMS_C_, ()))
KEY_MIXED_cCdD = KeyEntry(Level(KEYSYMS_cd, ()), Level(KEYSYMS_CD, ()))

TESTS = (
    # Single keysyms -> single keysyms
    TestEntry(
        KeyCode("Q", "AD01"),
        KEY_SIMPLE___,
        conflict=KEY_SIMPLE___,
        augment=KEY_SIMPLE___,
        override=KEY_SIMPLE___,
    ),
    TestEntry(
        KeyCode("W", "AD02"),
        KEY_SIMPLE___,
        conflict=KEY_SIMPLE_b_,
        augment=KEY_SIMPLE_b_,
        override=KEY_SIMPLE_b_,
    ),
    TestEntry(
        KeyCode("E", "AD03"),
        KEY_SIMPLE___,
        conflict=KEY_SIMPLE__B,
        augment=KEY_SIMPLE__B,
        override=KEY_SIMPLE__B,
    ),
    TestEntry(
        KeyCode("R", "AD04"),
        KEY_SIMPLE___,
        conflict=KEY_SIMPLE_bB,
        augment=KEY_SIMPLE_bB,
        override=KEY_SIMPLE_bB,
    ),
    TestEntry(
        KeyCode("T", "AD05"),
        KEY_SIMPLE_aA,
        conflict=KEY_SIMPLE___,
        augment=KEY_SIMPLE_aA,
        override=KEY_SIMPLE_aA,
    ),
    TestEntry(
        KeyCode("Y", "AD06"),
        KEY_SIMPLE_aA,
        conflict=KEY_SIMPLE_b_,
        augment=KEY_SIMPLE_aA,
        override=KEY_SIMPLE_bA,
    ),
    TestEntry(
        KeyCode("U", "AD07"),
        KEY_SIMPLE_aA,
        conflict=KEY_SIMPLE__B,
        augment=KEY_SIMPLE_aA,
        override=KEY_SIMPLE_aB,
    ),
    TestEntry(
        KeyCode("I", "AD08"),
        KEY_SIMPLE_aA,
        conflict=KEY_SIMPLE_bB,
        augment=KEY_SIMPLE_aA,
        override=KEY_SIMPLE_bB,
    ),
    # Single keysyms -> multiple keysyms
    TestEntry(
        KeyCode("A", "AC01"),
        KEY_SIMPLE___,
        conflict=KEY_MIXED_c__,
        augment=KEY_MIXED_c__,
        override=KEY_MIXED_c__,
    ),
    TestEntry(
        KeyCode("S", "AC02"),
        KEY_SIMPLE___,
        conflict=KEY_MIXED_c___,
        augment=KEY_MIXED_c___,
        override=KEY_MIXED_c___,
    ),
    TestEntry(
        KeyCode("D", "AC03"),
        KEY_SIMPLE___,
        conflict=KEY_MIXED__C_,
        augment=KEY_MIXED__C_,
        override=KEY_MIXED__C_,
    ),
    TestEntry(
        KeyCode("F", "AC04"),
        KEY_SIMPLE___,
        conflict=KEY_MIXED___C_,
        augment=KEY_MIXED___C_,
        override=KEY_MIXED___C_,
    ),
    TestEntry(
        KeyCode("G", "AC05"),
        KEY_SIMPLE___,
        conflict=KEY_MIXED_cC__,
        augment=KEY_MIXED_cC__,
        override=KEY_MIXED_cC__,
    ),
    TestEntry(
        KeyCode("H", "AC06"),
        KEY_SIMPLE___,
        conflict=KEY_MIXED_cCdD,
        augment=KEY_MIXED_cCdD,
        override=KEY_MIXED_cCdD,
    ),
    TestEntry(
        KeyCode("J", "AC07"),
        KEY_SIMPLE_aA,
        conflict=KEY_MIXED_c__,
        augment=KEY_SIMPLE_aA,
        override=KeyEntry(Level(KEYSYMS_c_, ()), Level(KEYSYMS_A, ())),
    ),
    TestEntry(
        KeyCode("K", "AC08"),
        KEY_SIMPLE_aA,
        conflict=KEY_MIXED_c___,
        augment=KEY_SIMPLE_aA,
        override=KEY_MIXED_c___,
    ),
    TestEntry(
        KeyCode("L", "AC09"),
        KEY_SIMPLE_aA,
        conflict=KEY_MIXED__C_,
        augment=KEY_SIMPLE_aA,
        override=KeyEntry(Level(KEYSYMS_a, ()), Level(KEYSYMS_C_, ())),
    ),
    TestEntry(
        KeyCode("SEMICOLON", "AC10"),
        KEY_SIMPLE_aA,
        conflict=KEY_MIXED___C_,
        augment=KEY_SIMPLE_aA,
        override=KEY_MIXED___C_,
    ),
    TestEntry(
        KeyCode("APOSTROPHE", "AC11"),
        KEY_SIMPLE_aA,
        conflict=KEY_MIXED_cC__,
        augment=KEY_SIMPLE_aA,
        override=KEY_MIXED_cC__,
    ),
    TestEntry(
        KeyCode("BACKSLASH", "AC12"),
        KEY_SIMPLE_aA,
        conflict=KEY_MIXED_cCdD,
        augment=KEY_SIMPLE_aA,
        override=KEY_MIXED_cCdD,
    ),
    # Multiple keysyms -> single keysyms
    # FIXME
    # Multiple keysyms -> multiple keysyms
    # FIXME
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
