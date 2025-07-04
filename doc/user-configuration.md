# User-configuration {#user-configuration}

This page describes how to add a *custom* layout or option so that it will be
parsed by libxkbcommon.

@note For an introduction to XKB and keymap components, please see
“@ref xkb-intro ""”.

@warning The below requires libxkbcommon as keymap compiler and
**does not work in X**.

@important An erroneous XKB configuration may make your keyboard unusable.
Therefore it is advised to try custom configurations safely using `xkbcli` tools;
see [debugging] for further details.

[debugging]: @ref testing-custom-config

@tableofcontents{html:2}

## Data locations {#user-config-locations}

libxkbcommon searches the following paths for XKB configuration files:
1. `$XDG_CONFIG_HOME/xkb/`, or `$HOME/.config/xkb/` if the `$XDG_CONFIG_HOME`
   environment variable is not defined. See the [XDG Base Directory Specification]
   for further details.
2. <div><!-- [HACK] Doxygen requires this extra div -->
   @deprecated `$HOME/.xkb/` as a *legacy* alternative to the previous XDG option.
   </div>
3. `$XKB_CONFIG_EXTRA_PATH` if set, otherwise `<sysconfdir>/xkb` (on most
   distributions this is `/etc/xkb`).
4. `$XKB_CONFIG_ROOT` if set, otherwise `<datadir>/X11/xkb/` (path defined by
   the `xkeyboard-config` package, on most distributions this is
   `/usr/share/X11/xkb`).

[XDG Base Directory Specification]: https://specifications.freedesktop.org/basedir-spec/latest/

A keymap created with `xkb_keymap::xkb_keymap_new_from_names2()` will look up
those paths in order until the required data is found.

@note Where libxkbcommon runs in a privileged context, only the system
(`<datadir>`) path is available.

Each directory should have one or more of the following subdirectories:
- `compat`
- `geometry` (libxkbcommon ignores this directory)
- `keycodes`
- `rules`
- `symbols`
- `types`

The majority of user-specific configurations involve modifying key symbols and
this is what this document focuses on. For use-cases where a user may need to
add new key types or compat entries the general approach remains the same. A
detailed description for how to add those types or compat entries is out of
scope for this document.

You should never need to add user-specific keycodes. Where a keycode is missing,
the addition should be filed in the upstream [xkeyboard-config] project.

[xkeyboard-config]: https://gitlab.freedesktop.org/xkeyboard-config/xkeyboard-config

## RMLVO vs KcCGST

Due to how XKB is configured, there is no such thing as a "layout" in XKB
itself, or, indeed, any of the rules, models, variant, options ([RMLVO]) described
in `struct xkb_rule_names`. [RMLVO] names are merely lookup keys in the
rules file provided by [xkeyboard-config] to map to the correct keycode, compat,
geometry (ignored by libxkbcommon), symbols and types ([KcCGST]). The [KcCGST]
data is the one used by XKB and libxkbcommon to map keys to actual symbols.

[RMLVO]: @ref RMLVO-intro
[KcCGST]: @ref KcCGST-intro

For example, a common [RMLVO] configuration is layout "us", variant "dvorak" and
option "terminate:ctrl_alt_bksp". Using the default rules file and model
this maps into the following [KcCGST] components:

```c
xkb_keymap {
	xkb_keycodes  { include "evdev+aliases(qwerty)"	};
	xkb_types     { include "complete"	};
	xkb_compat    { include "complete"	};
	xkb_symbols   { include "pc+us(dvorak)+inet(evdev)+terminate(ctrl_alt_bksp)"	};
	xkb_geometry  { include "pc(pc105)"	};
};
```

A detailed explanation of how rules files convert [RMLVO] to [KcCGST] is out of
scope for this document. See [the rules file](md_doc_rules-format.html) page
instead.


## Adding a layout {#user-config-layout}

Adding a layout requires that the user adds **symbols** in the correct location.

The default rules files (usually `evdev`) have a catch-all to map a layout, say
"foo", and a variant, say "bar", into the "bar" section in the file
`$xkb_base_dir/symbols/foo`.
This is sufficient to define a new keyboard layout. The example below defines
the keyboard layout "banana" with an optional variant "orange":

```
$ cat $XDG_CONFIG_HOME/xkb/symbols/banana
// Like a US layout but swap the top row so numbers are on Shift
default partial alphanumeric_keys
xkb_symbols "basic" {
    include "us(basic)"
    name[Group1]= "Banana (US)";

    key <AE01> { [ exclam,          1]     };
    key <AE02> { [ at,              2]     };
    key <AE03> { [ numbersign,      3]     };
    key <AE04> { [ dollar,          4]     };
    key <AE05> { [ percent,         5]     };
    key <AE06> { [ asciicircum,     6]     };
    key <AE07> { [ ampersand,       7]     };
    key <AE08> { [ asterisk,        8]     };
    key <AE09> { [ parenleft,       9]     };
    key <AE10> { [ parenright,      0]     };
    key <AE11> { [ underscore,      minus] };
    key <AE12> { [ plus,            equal] };
};

// Same as banana but map the euro sign to the 5 key
partial alphanumeric_keys
xkb_symbols "orange" {
    include "banana(basic)"
    name[Group1] = "Banana (Eurosign on 5)";
    include "eurosign(5)"
};
```

The `default` section is loaded when no variant is given. The first example
sections uses ``include`` to populate with a symbols list defined elsewhere
(here: section `basic` from the file `symbols/us`, aka. the default US keyboard
layout) and overrides parts of these symbols. The effect of this section is to
swap the numbers and symbols in the top-most row (compared to the US layout) but
otherwise use the US layout.

The "orange" variant uses the "banana" symbols and includes a different section
to define the `eurosign`. It does not specifically override any symbols.

The exact details of how `xkb_symbols` section works is out of scope for this
document; see: @ref keymap-text-format-v1-v2 "".

@remark This example uses a file name "banana" that should not clash with the
system files in `<datadir>/X11/xkb/symbols`. Using the same file name than
the system files is possible but may require special handling,
see: @ref user-config-system-file-names "".

## Adding an option

For technical reasons, options do **not** have a catch-all to map option names
to files and sections and must be specifically mapped by the user. This requires
a custom rules file. As the `evdev` ruleset is hardcoded in many clients, the
custom rules file must usually be named `evdev`.

```
$ cat $XDG_CONFIG_HOME/xkb/rules/evdev
// Mandatory to extend the
! include %S/evdev

! option = symbols
  custom:foo    = +custom(bar)
  custom:baz    = +other(baz)
```

The `include` statement includes the system-provided `evdev` ruleset. This
allows users to only override those options they need afterwards.

This rules file maps the [RMLVO] option "custom:foo" to the "bar" section in the
`symbols/custom` file and the "custom:baz" option to the "baz" section in the
`symbols/other` file. Note how the [RMLVO] option name may be different to the
file or section name.

@important The *order* of the options matter! In this example, `custom:foo` will
*always* be applied *before* `custom:baz` and both options will *always* be
applied *after* the system ones, even if the order is different in the [RMLVO]
configuration passed to libxkbcommon (e.g. with `xkbcli`). See the
[related section][options-order] in the rules documentation for further details.

[options-order]: @ref irrelevant-options-order

The files themselves are similar to the layout examples in the previous section:

```
$ cat $XDG_CONFIG_HOME/xkb/symbols/custom
// map the Tilde key to nothing on the first shift level
partial alphanumeric_keys
xkb_symbols "bar" {
    key <TLDE> {        [      VoidSymbol ]       };
};

$ cat $XDG_CONFIG_HOME/xkb/symbols/other
// map first key in bottom row (Z in the US layout) to k/K
partial alphanumeric_keys
xkb_symbols "baz" {
    key <AB01> {        [      k, K ]       };
};
```

With these in place, a user may select any layout/variant together with
the "custom:foo" and/or "custom:baz" options.

@remark This example uses file names "custom" and "other" that should not clash
with the system files in `<datadir>/X11/xkb/symbols`. Using the same file
name than the system files is possible but may require special handling,
see: @ref user-config-system-file-names "".

## Using system file names {#user-config-system-file-names}

The previous examples use custom keymap files with file name that do not clash
with the files in the *system* directory, `<datadir>/X11/xkb/`. It is possible
to add a custom variant using the *same* file name than the system file, e.g.
`$XDG_CONFIG_HOME/xkb/symbols/us` instead of `$XDG_CONFIG_HOME/xkb/symbols/banana`
for the example in @ref user-config-layout "".

However, it must then contains an *explicit default* section if the system file
has one, else it may break the keyboard setup by including a section of the
custom file instead of the system one. The default section should enforce that
the system default section is included. If one wanted to define only the "orange"
layout variant above, then the file would have been:

```
$ cat $XDG_CONFIG_HOME/xkb/symbols/us
default partial alphanumeric_keys xkb_symbols { include "us(basic)" };

partial alphanumeric_keys
xkb_symbols "orange" {
    include "us(basic)"
    name[Group1] = "Banana (Eurosign on 5)";
    include "eurosign(5)"
};
```

@since 1.9.0 The requirement for an explicit default section is not necessary
anymore: libxkbcommon will look up for the proper default section in the XKB
paths.

## Discoverable layouts

@warning The below requires `libxkbregistry` as XKB lookup tool and
**does not work where clients parse the XML file directly**.

The above sections apply only to the data files and require that the user knows
about the existence of the new entries. To make custom entries discoverable by
the configuration tools (e.g. the GNOME Control Center), the new entries must
also be added to the XML file that is parsed by `libxkbregistry`. In most cases,
this is the `evdev.xml` file in the rules directory. The example below shows the
XML file that would add the custom layout and custom options as outlined above
to the XKB registry:

```
$ cat $XDG_CONFIG_HOME/xkb/rules/evdev.xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE xkbConfigRegistry SYSTEM "xkb.dtd">
<xkbConfigRegistry version="1.1">
  <layoutList>
    <layout>
      <configItem>
        <name>banana</name>
        <shortDescription>ban</shortDescription>
        <description>Banana</description>
      </configItem>
      <variantList>
        <variant>
          <configItem>
            <name>orange</name>
            <shortDescription>or</shortDescription>
            <description>Orange (Banana)</description>
          </configItem>
        </variant>
      </variantList>
    </layout>
  </layoutList>
  <optionList>
    <group allowMultipleSelection="true">
      <configItem>
        <name>custom</name>
        <description>Custom options</description>
      </configItem>
      <option>
      <configItem>
        <name>custom:foo</name>
        <description>Map Tilde to nothing</description>
      </configItem>
      </option>
      <option>
      <configItem>
        <name>custom:baz</name>
        <description>Map Z to K</description>
      </configItem>
      </option>
    </group>
  </optionList>
</xkbConfigRegistry>
```

The default behavior of `libxkbregistry` ensures that the new layout and options
are added to the system-provided layouts and options.

For details on the XML format, see the DTD in `<datadir>/X11/xkb/rules/xkb.dtd`
and the system-provided XML files `<datadir>/X11/xkb/rules/*.xml`.

@note Depending on the desktop environment, it may require restarting the session
in order to make the configuration changes effective.

@note It is advised to try the custom configuration *before* restarting the
session using the various `xkbcli` tools. See [debugging] for further details.
