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
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        tests: tuple[Self, ...],
        groups: tuple[dict[str, tuple[str, str]], ...],
    ) -> None:
        path = root / f"test/data/symbols/{cls.symbols_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    symbols_file=cls.symbols_file,
                    tests_groups=tests,
                    groups=groups,
                    script=SCRIPT.relative_to(root),
                )
            )

    @classmethod
    def write_c_tests(
        cls,
        root: Path,
        jinja_env: jinja2.Environment,
        tests: tuple[Self, ...],
        groups: tuple[dict[str, tuple[str, str]], ...],
    ) -> None:
        path = root / f"test/{cls.test_file}"
        template_path = path.with_suffix(f"{path.suffix}.jinja")
        template = jinja_env.get_template(str(template_path.relative_to(root)))
        with path.open("wt", encoding="utf-8") as fd:
            fd.writelines(
                template.generate(
                    symbols_file=cls.symbols_file,
                    tests_groups=tests,
                    groups=groups,
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

KEYCODES = sorted(
    frozenset(t.key for g in TESTS for t in g.tests), key=lambda x: x._xkb
)
GREEK: tuple[tuple[str, str], ...] = (
    ("Greek_alpha", "Greek_ALPHA"),
    ("Greek_beta", "Greek_BETA"),
    ("Greek_gamma", "Greek_GAMMA"),
    ("Greek_delta", "Greek_DELTA"),
    ("Greek_epsilon", "Greek_EPSILON"),
    ("Greek_zeta", "Greek_ZETA"),
    ("Greek_eta", "Greek_ETA"),
    ("Greek_theta", "Greek_THETA"),
    ("Greek_iota", "Greek_IOTA"),
    ("Greek_kappa", "Greek_KAPPA"),
    ("Greek_lamda", "Greek_LAMDA"),
    ("Greek_lambda", "Greek_LAMBDA"),
    ("Greek_mu", "Greek_MU"),
    ("Greek_nu", "Greek_NU"),
    ("Greek_xi", "Greek_XI"),
    ("Greek_omicron", "Greek_OMICRON"),
    ("Greek_pi", "Greek_PI"),
    ("Greek_rho", "Greek_RHO"),
    ("Greek_sigma", "Greek_SIGMA"),
    ("Greek_tau", "Greek_TAU"),
    ("Greek_upsilon", "Greek_UPSILON"),
    ("Greek_phi", "Greek_PHI"),
    ("Greek_chi", "Greek_CHI"),
    ("Greek_psi", "Greek_PSI"),
    ("Greek_omega", "Greek_OMEGA"),
    ("Greek_alphaaccent", "Greek_ALPHAaccent"),
    ("Greek_epsilonaccent", "Greek_EPSILONaccent"),
    ("Greek_etaaccent", "Greek_ETAaccent"),
    ("Greek_iotaaccent", "Greek_IOTAaccent"),
    ("Greek_iotadieresis", "Greek_IOTAdieresis"),
    ("Greek_iotaaccentdieresis", "Greek_IOTAdiaeresis"),
    ("Greek_omicronaccent", "Greek_OMICRONaccent"),
    ("Greek_upsilonaccent", "Greek_UPSILONaccent"),
    ("Greek_upsilondieresis", "Greek_UPSILONdieresis"),
    ("Greek_omegaaccent", "Greek_OMEGAaccent"),
)
assert len(GREEK) >= len(KEYCODES)
GROUP2: dict[KeyCode, tuple[str, str]] = dict(zip(KEYCODES, GREEK))
BRAILLE = (
    ("braille_blank", "braille_dots_36"),
    ("braille_dots_1", "braille_dots_136"),
    ("braille_dots_2", "braille_dots_236"),
    ("braille_dots_12", "braille_dots_1236"),
    ("braille_dots_3", "braille_dots_46"),
    ("braille_dots_13", "braille_dots_146"),
    ("braille_dots_23", "braille_dots_246"),
    ("braille_dots_123", "braille_dots_1246"),
    ("braille_dots_4", "braille_dots_346"),
    ("braille_dots_14", "braille_dots_1346"),
    ("braille_dots_24", "braille_dots_2346"),
    ("braille_dots_124", "braille_dots_12346"),
    ("braille_dots_34", "braille_dots_56"),
    ("braille_dots_134", "braille_dots_156"),
    ("braille_dots_234", "braille_dots_256"),
    ("braille_dots_1234", "braille_dots_1256"),
    ("braille_dots_5", "braille_dots_356"),
    ("braille_dots_15", "braille_dots_1356"),
    ("braille_dots_25", "braille_dots_2356"),
    ("braille_dots_125", "braille_dots_12356"),
    ("braille_dots_35", "braille_dots_456"),
    ("braille_dots_135", "braille_dots_1456"),
    ("braille_dots_235", "braille_dots_2456"),
    ("braille_dots_1235", "braille_dots_12456"),
    ("braille_dots_45", "braille_dots_3456"),
    ("braille_dots_145", "braille_dots_13456"),
    ("braille_dots_245", "braille_dots_23456"),
    ("braille_dots_1245", "braille_dots_123456"),
    ("braille_dots_345", "braille_dots_7"),
    ("braille_dots_1345", "braille_dots_17"),
    ("braille_dots_2345", "braille_dots_27"),
    ("braille_dots_12345", "braille_dots_127"),
    ("braille_dots_6", "braille_dots_37"),
    ("braille_dots_16", "braille_dots_137"),
    ("braille_dots_26", "braille_dots_237"),
    ("braille_dots_126", "braille_dots_1237"),
)
assert len(BRAILLE) >= len(KEYCODES)
GROUP3: dict[KeyCode, tuple[str, str]] = dict(zip(KEYCODES, BRAILLE))
GROUPS = (GROUP2, GROUP3)

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
    TestEntry.write_symbols(
        root=args.root, jinja_env=jinja_env, tests=TESTS, groups=GROUPS
    )
    TestEntry.write_c_tests(
        root=args.root, jinja_env=jinja_env, tests=TESTS, groups=GROUPS
    )
