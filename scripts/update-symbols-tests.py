#!/usr/bin/env python3

"""
This script generate tests for symbols.
"""

import argparse
import dataclasses
import itertools
from abc import ABCMeta, abstractmethod
from dataclasses import dataclass
from enum import IntFlag
from pathlib import Path
from typing import Any, ClassVar, Iterable, Iterator, Self

import jinja2

SCRIPT = Path(__file__)


@dataclass(frozen=True)
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
        return "+".join(m.name for m in self)


class Action(metaclass=ABCMeta):
    @abstractmethod
    def __bool__(self) -> bool: ...

    @classmethod
    def parse(cls, raw: Any) -> Self:
        if raw is None:
            return GroupAction.parse(None)
        elif isinstance(raw, Modifier):
            return ModAction.parse(raw)
        elif isinstance(raw, int):
            return GroupAction.parse(raw)
        else:
            raise ValueError(raw)

    @abstractmethod
    def action_to_keysym(self, index: int, level: int) -> Keysym: ...


@dataclass
class GroupAction(Action):
    """
    SetGroup or NoAction
    """

    group: int
    keysyms: ClassVar[dict[tuple[int, int, int], Keysym]] = {
        (2, 0, 0): Keysym("a"),
        (2, 0, 1): Keysym("A"),
        (2, 1, 0): Keysym("b"),
        (2, 1, 1): Keysym("B"),
        (3, 0, 0): Keysym("Greek_alpha"),
        (3, 0, 1): Keysym("Greek_ALPHA"),
        (3, 1, 0): Keysym("Greek_beta"),
        (3, 1, 1): Keysym("Greek_BETA"),
    }

    def __str__(self) -> str:
        if self.group > 0:
            return f"SetGroup(group={self.group})"
        else:
            return "NoAction()"

    def __bool__(self) -> bool:
        return bool(self.group)

    @classmethod
    def parse(cls, raw: int | None) -> Self:
        if not raw:
            return cls(0)
        else:
            return cls(raw)

    def action_to_keysym(self, index: int, level: int) -> Keysym:
        if not self.group:
            return NoSymbol
        else:
            if (
                keysym := self.keysyms.get((self.group % 4, index % 2, level % 2))
            ) is None:
                raise ValueError((self, index, level))
            return keysym


@dataclass
class ModAction(Action):
    """
    SetMod or NoAction
    """

    mods: Modifier
    keysyms: ClassVar[dict[tuple[Modifier, int, int], Keysym]] = {
        (Modifier.Control, 0, 0): Keysym("x"),
        (Modifier.Control, 0, 1): Keysym("X"),
        (Modifier.Control, 1, 0): Keysym("y"),
        (Modifier.Control, 1, 1): Keysym("Y"),
        (Modifier.Mod5, 0, 0): Keysym("Greek_xi"),
        (Modifier.Mod5, 0, 1): Keysym("Greek_XI"),
        (Modifier.Mod5, 1, 0): Keysym("Greek_upsilon"),
        (Modifier.Mod5, 1, 1): Keysym("Greek_UPSILON"),
    }

    def __str__(self) -> str:
        if self.mods is Modifier.NoModifier:
            return "NoAction()"
        else:
            return f"SetMods(mods={self.mods})"

    def __bool__(self) -> bool:
        return self.mods is Modifier.NoModifier

    @classmethod
    def parse(cls, raw: Modifier | None) -> Self:
        if not raw:
            return cls(Modifier.NoModifier)
        else:
            return cls(raw)

    def action_to_keysym(self, index: int, level: int) -> Keysym:
        if self.mods is Modifier.NoModifier:
            return NoSymbol
        else:
            if (keysym := self.keysyms.get((self.mods, index % 2, level % 2))) is None:
                raise ValueError((self, index, level))
            return keysym


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
    def has_empty_symbols(cls, keysyms: tuple[Keysym, ...]) -> bool:
        return all(ks == NoSymbol for ks in keysyms)

    @property
    def empty_symbols(self) -> bool:
        return self.has_empty_symbols(self.keysyms)

    @property
    def keysyms_c(self) -> str:
        if not self.keysyms and self.actions:
            return self._c(
                NoSymbol.c, tuple(itertools.repeat(NoSymbol, len(self.actions)))
            )
        return self._c(NoSymbol.c, self.keysyms)

    @property
    def keysyms_xkb(self) -> str:
        return self._xkb(NoSymbol, self.keysyms)

    @classmethod
    def has_empty_actions(cls, actions: tuple[Action, ...]) -> bool:
        return not any(actions)

    @property
    def empty_actions(self) -> bool:
        return self.has_empty_actions(self.actions)

    @property
    def actions_xkb(self) -> str:
        return self._xkb("NoAction()", self.actions)

    @classmethod
    def Keysyms(cls, *keysyms: str | None) -> Self:
        return cls.Mix(keysyms, ())

    @classmethod
    def Actions(cls, *actions: int | Modifier | None) -> Self:
        return cls.Mix((), actions)

    @classmethod
    def Mix(
        cls, keysyms: tuple[str | None, ...], actions: tuple[int | Modifier | None,]
    ) -> Self:
        return cls(tuple(map(Keysym.parse, keysyms)), tuple(map(Action.parse, actions)))

    def add_keysyms(self, keep_actions: bool, level: int) -> Self:
        return self.__class__(
            keysyms=tuple(
                a.action_to_keysym(index=k, level=level)
                for k, a in enumerate(self.actions)
            ),
            actions=self.actions if keep_actions else (),
        )

    @property
    def target_group(self) -> int:
        for a in self.actions:
            if isinstance(a, GroupAction) and a.group > 1:
                return a.group
        else:
            return 0

    @property
    def target_level(self) -> int:
        for a in self.actions:
            if isinstance(a, ModAction) and a.mods:
                match a.mods:
                    case Modifier.LevelThree:
                        return 2
                    case _:
                        return 0
        else:
            return 0


@dataclass
class KeyEntry:
    levels: tuple[Level, ...]

    def __init__(self, *levels: Level):
        self.levels = levels

    @property
    def xkb(self) -> Iterator[str]:
        keysyms = tuple(l.keysyms for l in self.levels)
        has_keysyms = any(not Level.has_empty_symbols(s) for s in keysyms)
        no_keysyms = all(not s for s in keysyms)
        actions = tuple(l.actions for l in self.levels)
        has_actions = any(not Level.has_empty_actions(a) for a in actions)
        if has_keysyms or (not no_keysyms and not has_actions):
            yield "symbols=["
            yield ", ".join(l.keysyms_xkb for l in self.levels)
            yield "]"
        if has_actions or no_keysyms:
            if has_keysyms:
                yield ", "
            yield "actions=["
            yield ", ".join(l.actions_xkb for l in self.levels)
            yield "]"

    def add_keysyms(self, keep_actions: bool) -> Self:
        return self.__class__(
            *(
                l.add_keysyms(keep_actions=keep_actions, level=k)
                for k, l in enumerate(self.levels)
            )
        )


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
    group_keysyms: ClassVar[tuple[tuple[str, str], ...]] = (
        ("Ukrainian_i", "Ukrainian_I", "Ukrainian_yi", "Ukrainian_YI"),
        ("schwa", "SCHWA", "dead_schwa", "dead_SCHWA"),
    )

    def add_keysyms(self, keep_actions: bool) -> Self:
        return dataclasses.replace(
            self,
            base=self.base.add_keysyms(keep_actions=keep_actions),
            update=self.update.add_keysyms(keep_actions=keep_actions),
            augment=self.augment.add_keysyms(keep_actions=keep_actions),
            override=self.override.add_keysyms(keep_actions=keep_actions),
        )

    @classmethod
    def alt_keysym(cls, group: int, level: int) -> Keysym:
        return Keysym(cls.group_keysyms[group % 2][level % 4])

    @classmethod
    def alt_keysyms(cls, group: int) -> Iterator[Keysym]:
        for keysym in cls.group_keysyms[group % 2]:
            yield Keysym(keysym)

    @classmethod
    def write_symbols(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        tests: tuple[Self, ...],
        keycodes: Iterable[KeyCode],
    ) -> None:
        path = root / f"test/data/symbols/{cls.symbols_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    symbols_file=cls.symbols_file,
                    keycodes=keycodes,
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

    def add_keysyms(self, name: str, keep_actions: bool) -> Self:
        return dataclasses.replace(
            self,
            name=name,
            tests=tuple(
                t.add_keysyms(keep_actions=keep_actions)
                for t in TESTS_ACTIONS_ONLY.tests
            ),
        )


TESTS_ACTIONS_ONLY = TestGroup(
    "actions_only",
    (
        # Single keysyms -> single keysyms
        TestEntry(
            KeyCode("Q", "AD01"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(None)),
            override=KeyEntry(Level.Actions(None), Level.Actions(None)),
        ),
        TestEntry(
            KeyCode("W", "AD02"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(3), Level.Actions(None)),
            override=KeyEntry(Level.Actions(3), Level.Actions(None)),
        ),
        TestEntry(
            KeyCode("E", "AD03"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(3)),
            override=KeyEntry(Level.Actions(None), Level.Actions(3)),
        ),
        TestEntry(
            KeyCode("R", "AD04"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(3), Level.Actions(3)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
        ),
        TestEntry(
            KeyCode("T", "AD05"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(2)),
        ),
        TestEntry(
            KeyCode("Y", "AD06"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(2)),
        ),
        TestEntry(
            KeyCode("U", "AD07"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(3)),
        ),
        TestEntry(
            KeyCode("I", "AD08"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
        ),
        # Single keysyms -> multiple keysyms
        TestEntry(
            KeyCode("A", "AC01"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
        ),
        TestEntry(
            KeyCode("S", "AC02"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, None)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
        ),
        TestEntry(
            KeyCode("D", "AC03"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            override=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
        ),
        TestEntry(
            KeyCode("F", "AC04"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(None, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            override=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
        ),
        TestEntry(
            KeyCode("G", "AC05"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
        ),
        TestEntry(
            KeyCode("H", "AC06"),
            KeyEntry(Level.Actions(None), Level.Actions(None)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
        ),
        TestEntry(
            KeyCode("J", "AC07"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(2)),
        ),
        TestEntry(
            KeyCode("K", "AC08"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(2)),
        ),
        TestEntry(
            KeyCode("L", "AC09"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(3, None)),
        ),
        TestEntry(
            KeyCode("SEMICOLON", "AC10"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(None, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(2), Level.Actions(3, None)),
        ),
        TestEntry(
            KeyCode("APOSTROPHE", "AC11"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(3, None)),
        ),
        TestEntry(
            KeyCode("BACKSLASH", "AC12"),
            KeyEntry(Level.Actions(2), Level.Actions(2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2)),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
        ),
        # Multiple keysyms -> multiple keysyms
        TestEntry(
            KeyCode("Z", "AB01"),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None)),
            update=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            augment=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            override=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
        ),
        TestEntry(
            KeyCode("X", "AB02"),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            augment=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
        ),
        TestEntry(
            KeyCode("C", "AB03"),
            KeyEntry(Level.Actions(None, None), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(2, Modifier.Control),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
        ),
        TestEntry(
            KeyCode("V", "AB04"),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            augment=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            override=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
        ),
        TestEntry(
            KeyCode("B", "AB05"),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            augment=KeyEntry(
                Level.Actions(2, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 2),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
        ),
        TestEntry(
            KeyCode("N", "AB06"),
            KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(2, Modifier.Control)
            ),
            update=KeyEntry(
                Level.Actions(None, None), Level.Actions(3, Modifier.LevelThree)
            ),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(2, Modifier.Control)
            ),
            override=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(3, Modifier.LevelThree),
            ),
        ),
        TestEntry(
            KeyCode("M", "AB07"),
            KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(Modifier.Control, 2)
            ),
            update=KeyEntry(Level.Actions(3, None), Level.Actions(None, 3)),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control), Level.Actions(Modifier.Control, 2)
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.Control), Level.Actions(Modifier.Control, 3)
            ),
        ),
        TestEntry(
            KeyCode("COMMA", "AB08"),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None, None)),
            update=KeyEntry(Level.Actions(None, None, None), Level.Actions(None, None)),
            augment=KeyEntry(
                Level.Actions(None, None), Level.Actions(None, None, None)
            ),
            override=KeyEntry(
                Level.Actions(None, None), Level.Actions(None, None, None)
            ),
        ),
        TestEntry(
            KeyCode("DOT", "AB09"),
            KeyEntry(Level.Actions(None, None), Level.Actions(None, None, None)),
            update=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            augment=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            override=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
        ),
        TestEntry(
            KeyCode("SLASH", "AB10"),
            KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            update=KeyEntry(Level.Actions(None, None, None), Level.Actions(None, None)),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            override=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
        ),
        TestEntry(
            KeyCode("RO", "AB11"),
            KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            update=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
            augment=KeyEntry(
                Level.Actions(2, Modifier.Control),
                Level.Actions(Modifier.Control, None, 2),
            ),
            override=KeyEntry(
                Level.Actions(3, None, Modifier.LevelThree),
                Level.Actions(Modifier.LevelThree, 3),
            ),
        ),
        # Multiple keysyms -> single keysyms
        TestEntry(
            KeyCode("GRAVE", "TLDE"),
            KeyEntry(Level.Actions(None, None), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(
                Level.Actions(None, None), Level.Actions(2, Modifier.Control)
            ),
            override=KeyEntry(
                Level.Actions(None, None), Level.Actions(2, Modifier.Control)
            ),
        ),
        TestEntry(
            KeyCode("1", "AE01"),
            KeyEntry(Level.Actions(None, None), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(3), Level.Actions(2, Modifier.Control)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
        ),
        TestEntry(
            KeyCode("2", "AE02"),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(Level.Actions(None), Level.Actions(None)),
            augment=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            override=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
        ),
        TestEntry(
            KeyCode("3", "AE03"),
            KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            update=KeyEntry(Level.Actions(3), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2, None), Level.Actions(None, 2)),
            override=KeyEntry(Level.Actions(3), Level.Actions(3)),
        ),
        # Mix
        TestEntry(
            KeyCode("4", "AE04"),
            KeyEntry(Level.Actions(2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
            augment=KeyEntry(Level.Actions(2), Level.Actions(3, Modifier.LevelThree)),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3, Modifier.LevelThree),
            ),
        ),
        TestEntry(
            KeyCode("5", "AE05"),
            KeyEntry(Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2, Modifier.Control), Level.Actions(3)),
            override=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
        ),
        TestEntry(
            KeyCode("6", "AE06"),
            KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
            augment=KeyEntry(Level.Actions(2), Level.Actions(2, Modifier.Control)),
            override=KeyEntry(Level.Actions(3, Modifier.LevelThree), Level.Actions(3)),
        ),
    ),
)

TESTS_KEYSYMS_ONLY = TESTS_ACTIONS_ONLY.add_keysyms(
    name="keysyms_only", keep_actions=False
)
# Create a mix keysyms/actions
TESTS_KEYSYMS_AND_ACTIONS1 = TESTS_ACTIONS_ONLY.add_keysyms(
    name="keysyms_and_actions_auto", keep_actions=True
)

# Further mixes between actions and keysyms
TESTS_KEYSYMS_AND_ACTIONS2 = TestGroup(
    "keysyms_and_actions_extras",
    (
        TestEntry(
            KeyCode("GRAVE", "TLDE"),
            KeyEntry(Level.Keysyms("a"), Level.Actions(2)),
            update=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
            augment=KeyEntry(Level.Mix(("a"), (3,)), Level.Mix(("X",), (2,))),
            override=KeyEntry(Level.Mix(("a"), (3,)), Level.Mix(("X",), (2,))),
        ),
        TestEntry(
            KeyCode("1", "AE01"),
            KeyEntry(Level.Keysyms("a"), Level.Actions(2)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree), Level.Keysyms("X", "Y")
            ),
            augment=KeyEntry(Level.Keysyms("a"), Level.Actions(2)),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree), Level.Keysyms("X", "Y")
            ),
        ),
        # Multiple keysyms/actions –> single
        TestEntry(
            KeyCode("2", "AE02"),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
            augment=KeyEntry(
                Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)
            ),
            override=KeyEntry(Level.Actions(3), Level.Keysyms("X")),
        ),
        # Multiple keysyms/actions –> multiple (xor)
        TestEntry(
            KeyCode("3", "AE03"),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree), Level.Keysyms("X", "Y")
            ),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.Control)),
            ),
        ),
        # Multiple keysyms/actions –> multiple (mix)
        TestEntry(
            KeyCode("4", "AE04"),
            KeyEntry(Level.Keysyms("a", None), Level.Actions(2, None)),
            update=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.LevelThree)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
        ),
        TestEntry(
            KeyCode("5", "AE05"),
            KeyEntry(Level.Keysyms("a", "b"), Level.Actions(2, Modifier.Control)),
            update=KeyEntry(
                Level.Mix(("x", None), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, None)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "b"), (3, Modifier.LevelThree)),
                Level.Mix(("X", "Y"), (3, Modifier.Control)),
            ),
        ),
        # Multiple (mix) –> multiple keysyms/actions
        TestEntry(
            KeyCode("6", "AE06"),
            KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            update=KeyEntry(Level.Keysyms("x", None), Level.Actions(3, None)),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "b"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (3, Modifier.Control)),
            ),
        ),
        TestEntry(
            KeyCode("7", "AE07"),
            KeyEntry(
                Level.Mix(("a", None), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, None)),
            ),
            update=KeyEntry(
                Level.Keysyms("x", "y"), Level.Actions(3, Modifier.LevelThree)
            ),
            augment=KeyEntry(
                Level.Mix(("a", "y"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (2, Modifier.LevelThree)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "y"), (2, Modifier.Control)),
                Level.Mix(("A", "B"), (3, Modifier.LevelThree)),
            ),
        ),
        # Multiple (mix) –> multiple (mix)
        TestEntry(
            KeyCode("8", "AE08"),
            KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix((None, "B"), (2, None)),
            ),
            update=KeyEntry(
                Level.Mix((None, "y"), (3, None)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", "b"), (2, Modifier.Control)),
                Level.Mix(("X", "B"), (2, Modifier.LevelThree)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "y"), (3, Modifier.Control)),
                Level.Mix(("X", "Y"), (3, Modifier.LevelThree)),
            ),
        ),
        TestEntry(
            KeyCode("9", "AE09"),
            KeyEntry(
                Level.Mix(("a", None), (2, None)),
                Level.Mix((None, "B"), (None, Modifier.Control)),
            ),
            update=KeyEntry(
                Level.Mix((None, "y"), (None, Modifier.LevelThree)),
                Level.Mix(("X", None), (3, None)),
            ),
            augment=KeyEntry(
                Level.Mix(("a", "y"), (2, Modifier.LevelThree)),
                Level.Mix(("X", "B"), (3, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("a", "y"), (2, Modifier.LevelThree)),
                Level.Mix(("X", "B"), (3, Modifier.Control)),
            ),
        ),
        # Mismatch count with mix
        TestEntry(
            KeyCode("Q", "AD01"),
            KeyEntry(
                Level.Keysyms("a"),
                Level.Keysyms("A", "B"),
            ),
            update=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3),
            ),
            augment=KeyEntry(
                Level.Keysyms("a"),
                Level.Keysyms("A", "B"),
            ),
            override=KeyEntry(
                Level.Actions(3, Modifier.LevelThree),
                Level.Actions(3),
            ),
        ),
        TestEntry(
            KeyCode("W", "AD02"),
            KeyEntry(
                Level.Actions(3),
                Level.Actions(3, Modifier.LevelThree),
            ),
            update=KeyEntry(
                Level.Keysyms("A", "B"),
                Level.Keysyms("a"),
            ),
            augment=KeyEntry(
                Level.Actions(3),
                Level.Actions(3, Modifier.LevelThree),
            ),
            override=KeyEntry(
                Level.Keysyms("A", "B"),
                Level.Keysyms("a"),
            ),
        ),
        TestEntry(
            KeyCode("E", "AD03"),
            KeyEntry(
                Level.Mix(("a",), (2,)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            update=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X",), (3,)),
            ),
            augment=KeyEntry(
                Level.Mix(("a",), (2,)),
                Level.Mix(("A", "B"), (2, Modifier.Control)),
            ),
            override=KeyEntry(
                Level.Mix(("x", "y"), (3, Modifier.LevelThree)),
                Level.Mix(("X",), (3,)),
            ),
        ),
    ),
)

TESTS = (
    TESTS_KEYSYMS_ONLY,
    TESTS_ACTIONS_ONLY,
    TESTS_KEYSYMS_AND_ACTIONS1,
    TESTS_KEYSYMS_AND_ACTIONS2,
)

KEYCODES = sorted(
    frozenset(t.key for g in TESTS for t in g.tests), key=lambda x: x._xkb
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
    jinja_env.globals["alt_keysym"] = TestEntry.alt_keysym
    jinja_env.globals["alt_keysyms"] = TestEntry.alt_keysyms
    TestEntry.write_symbols(
        root=args.root, jinja_env=jinja_env, tests=TESTS, keycodes=KEYCODES
    )
    TestEntry.write_c_tests(root=args.root, jinja_env=jinja_env, tests=TESTS)
