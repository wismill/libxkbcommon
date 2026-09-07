@page coding-conventions Coding conventions

<!--
Initial version based on libinput’s `CODING_STYLE.md`
See: https://gitlab.freedesktop.org/libinput/libinput/-/blob/main/CODING_STYLE.md
-->

@tableofcontents{html:2}

# Coding style

<dl>
<dt>Indentation</dt>
<dd>*Spaces*, 4 characters wide.</dd>
<dt>Max line width</dt>
<dd>*80* characters. Do not break up printed strings though.</dd>
<dt>Long lines</dt>
<dd>
Break up long lines at logical groupings, one line for each logical group.

```c
int a = somelongname() +
        someotherlongname();

if (a < 0 &&
    (b > 20 & d < 10) &&
    d != 0.0)


somelongfunctioncall(arg1,
                     arg2,
                     arg3);
```
</dd>
<dt>Function declarations</dt>
<dd>
Return type on separate line, `{}` on separate line, arguments broken up as above.

```c
static inline int
foobar(int a, int b)
{
}

void
somenamethatiswaytoolong(int a,
                         int b,
                         int c)
{
}
```

Use `const` parameters whenever relevant to indicate ownership.
</dd>
<dt>Comments</dt>
<dd>`/* */` comments only, no `//` comments.</dd>
<dt>Naming conventions</dt>
<dd>
`variable_name`, not `VariableName` or `variableName`. Same for functions.

Note that the code may contain legacy X.Org naming convention. These bits should
be migrated some day, while keeping a reference to the original X.Org’s functions.

@todo Naming patterns
<!-- blank required by Doxygen -->

</dd>
<dt>Typedefs</dt>
<dd>No typedefs of structs, enums, unions</dd>
<dt>Compiler warning</dt>
<dd>It needs to be fixed!</dd>
<dt>Static checker warning</dt>
<dd>It needs to be fixed or commented!</dd>
<dt>Variable declaration</dt>
<dd>
Declare variables when they are used first and try to keep them as local as possible.

Exception: basic loop variables, e.g. `for (int i = 0; ...)` should always be
declared inside the loop even where multiple loops exist.

Use `const` whenever relevant.

```c
int a;

if (foo) {
    const int b = 10;

    a = get_value();
    usevalue(a, b);
}

if (bar) {
    a = get_value();
    useit(a);
}

const int c = a * 100;
useit(c);
```
</dd>
<dt>Variable initialization</dt>
<dd>
Avoid uninitialized variables where possible, declare them late instead.
Note that legacy code predates this style; fix it whenever relevant.

- Wrong:

  ```c
  int *a;
  int b = 7;

  ... some code ...

  a = zalloc(32);
  ```
- Right:

  ```c
  int b = 7;
  ... some code ...

  int *a = zalloc(32);
  ```
</dd>
<dt>Declaration blocks</dt>
<dd>
Avoid calling non-obvious functions inside declaration blocks for multiple
variables.

- Bad:

  ```c
  {
      int a = 7;
      int b = some_complicated_function();
      int *c = zalloc(32);
  }
  ```
- Better:
  ```c
  {
      int a = 7;
      int *c = zalloc(32);

      int b = some_complicated_function();
  }
  ```

There is a bit of gut-feeling involved with this, but the goal is to make
the variable values immediately recognizable.
</dd>
<dt>`if/else`</dt>
<dd>
`{` on the same line, no curly braces if both blocks are a single statement.
If either `if` or `else` block are multiple statements, both must have curly braces.

```c
if (foo) {
    blah();
    bar();
} else {
    a = 10;
}
```
</dd>
<dt>Public functions</dt>
<dd>
MUST be doxygen-commented and declared in `xkbcommon.map`.
</dd>
<dt>Includes</dt>
<dd>
`#include "config.h"` comes first, followed by system headers, followed by external library headers, followed by internal headers.
Sort alphabetically where it makes sense (specifically system headers).

```c
#include "config.h"

#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>

#include "keymap.h"
```
</dd>
<dt>`goto`</dt>
<dd>
`goto` jumps only to the end of the function, and only for good reasons
(usually cleanup). `goto` never jumps backwards.
</dd>
<dt>Boolean</dt>
<dd>
Use `<stdbool.h>`’s `bool` for booleans within the library (instead of `int`).
Exception: the legacy API.
</dd>
</dl>

# Git commit message requirements

Our CI will check the commit messages for a few requirements. Below is the
list of what we expect from a git commit.

## Commit message content

A [good commit message](http://who-t.blogspot.com/2009/12/on-commit-messages.html)
needs to answer three questions:

- Why is it necessary? It may fix a bug, it may add a feature, it may
  improve performance, reliabilty, stability, or just be a change for the
  sake of correctness.
- How does it address the issue? For short obvious patches this part can be
  omitted, but it should be a high level description of what the approach
  was.
- What effects does the patch have? (In addition to the obvious ones, this
  may include benchmarks, side effects, etc.)

These three questions establish the context for the actual code changes, put
reviewers and others into the frame of mind to look at the diff and check if
the approach chosen was correct. A good commit message also helps
maintainers to decide if a given patch is suitable for stable branches or
inclusion in a distribution.

## Commit message format

The canonical git commit message format is:

```
one line as the subject line with a high-level note

full explanation of the patch follows after an empty line. This explanation
can be multiple paragraphs and is largely free-form. Markdown is not
supported.

You can include extra data where required like:
- benchmark one says 10s
- benchmark two says 12s
```

The subject line is the first thing everyone sees about this commit, so make
sure it’s on point.

## Commit message technical requirements

<!-- TODO
- The commit message should use present tense (not past tense). Do write
  "change foo to bar", not "changed foo to bar".
-->
- The text width of the commit should be 78 chars or less, especially the
  subject line.
- The author must be the name you usually identify as and email address. We do
  not accept the default `@users.noreply` gitlab addresses.
  ```
  git config --global user.name Your Name
  git config --global user.email your@email
  ```
