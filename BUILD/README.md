# XScript Compiler
### Version 0.8 BETA
#### X3 Farnham's Legacy Script Compiler

Website: https://www.xpluginmanager.co.uk/xscript/

---

## Overview

XScript Compiler translates human-readable XScript source files (`.xs`) into the XML script format used by X3 Farnham's Legacy. It provides full type checking, meaningful error messages with source location, and supports the complete XScript language including object methods, conditions, arrays, labels, local-scoped user functions, and mod-added custom commands.

---

## Requirements

- `default_data.dat` (or `x3fl.dat`) — the compiled game data file containing all function definitions, constants, and datatypes. This file is generated from the `x3fl.xml` definition file using the data compiler.

---

## Usage

```
XScriptCompiler.exe --load_data <datafile> --compile <input.xs> --out <output.xml>
XScriptCompiler.exe --load_data <datafile> --compile <input.xs> --out <output.xml> --define:MYSYMBOL
XScriptCompiler.exe --builddata <x3fl.xml> --out <output.dat>
XScriptCompiler.exe --load_data <datafile> --decompile <input.xml> --out <output.xs>
```

### Compile a script

```
XScriptCompiler.exe --load_data default_data.dat --compile myscript.xs --out myscript.xml
```

**Arguments:**

| Argument | Description |
|----------|-------------|
| `--load_data <file>` | Path to the compiled game data file (`default_data.dat` or `x3fl.dat`) |
| `--compile <file>` | Path to the XScript source file (`.xs`) to compile |
| `--out <file>` | Path for the compiled XML script output |
| `--define:NAME` | Pre-define a symbol for use with `#ifdef`. Multiple `--define:` arguments are supported. |

---

### Build the data file

Compiles `x3fl.xml` and the associated game data files into a binary `.dat` file. This must be done before compiling or decompiling scripts, and must be re-run whenever `x3fl.xml` is updated.

```
XScriptCompiler.exe --builddata x3fl.xml --out default_data.dat
```

**Arguments:**

| Argument | Description |
|----------|-------------|
| `--builddata <file>` | Path to the XScript definition XML file (`x3fl.xml`) |
| `--out <file>` | Path for the compiled binary data file output |

**Required game files:**

The build process reads several files from the X3 Farnham's Legacy game data directory. These must be present in a `Data\` subdirectory relative to the working directory.

| File | Description |
|------|-------------|
| `Data\0001-L044.xml` | Game text file — display names and descriptions for object commands and ware types |
| `Data\<name>.txt` | Ware type files — one per `<WareType>` entry in `x3fl.xml` |

A typical directory layout:

```
XScriptCompiler.exe
x3fl.xml
Data\
    0001-L044.xml
    TShips.txt
    TDocks.txt
    TMissiles.txt
    ... (other ware type files as referenced in x3fl.xml)
```

---

### Decompile a script

Converts a compiled X3 XML script back into human-readable XScript source.

```
XScriptCompiler.exe --load_data default_data.dat --decompile myscript.xml --out myscript.xs
```

**Arguments:**

| Argument | Description |
|----------|-------------|
| `--load_data <file>` | Path to the compiled game data file |
| `--decompile <file>` | Path to the compiled XML script to decompile |
| `--out <file>` | Path for the XScript source output |
| `--usenamespace` | Emit namespaced function calls (e.g. `Utils::random`) where a mapping exists |

The decompiler outputs v0.8 syntax — scripts are wrapped in a `function main(...)` block with argument descriptions preserved as comments, and `#VERSION`/`#DESCRIPTION`/`#COMMAND` directives emitted before the function.

---

## Error Output

```
Compile Error [#N]:   [filename:line:col]  - message text
    <source line shown here>
       ^

Compile Warning [#N]: [filename:line:col]  - message text
    <source line shown here>
       ^
```

Errors prevent the output file from being written. Warnings are informational — the script is still compiled.

---

## Language Reference

### Script Structure (v0.8)

Every v0.8 XScript file follows this structure:

```xscript
// Optional metadata directives — must appear before the function
#DESCRIPTION "My plugin script"
#VERSION 1
#COMMAND 0

// Optional: user-defined functions and subs (before or after main)
function DATATYPE_INT getSectorCount(VARSECTOR $sector)
{
    return(1);
}

sub logMessage()
{
    incomingMessage($logText, PlayerLog::Alert, TRUE);
}

// Required: the main function — script entry point
function main(VARSECTOR $sector, VARSHIP $ship)
{
    $count = getSectorCount($sector);
    gosub logMessage;
    return(null);
}
```

The `function main(...)` wrapper is **required**. All executable code must be inside `main` or a user-defined function/sub. Code placed outside any function block is a compile error.

---

### Line Termination

All statements end with a semicolon `;`:

```xscript
$x = 10;
incomingMessage("Hello", PlayerLog::Alert, TRUE);
```

### Comments

```xscript
// Single line comment

/* Multi-line comment
   anything between these is ignored */
```

### Variables

Variables begin with `$`. The character after `$` must be a letter; subsequent characters may be letters, numbers, underscores, or periods:

```xscript
$count = 0;
$my.variable = "hello";
$ship2 = PLAYERSHIP;
```

### Numbers

X3 scripts only support **integer** values — no floating point:

```xscript
$x = 42;
$y = -10;
```

### Assignments

```xscript
$variable = 10;
$array[1] = 10;
```

**Chained (double) assignment:**

```xscript
$x = $y = -1;
$x = $y = random(2);
```

### Expression Operators

| Operator | Description |
|----------|-------------|
| `+` | Addition (integers) or concatenation (strings) |
| `-` | Subtraction |
| `*` | Multiply |
| `/` | Divide |
| `%` | Modulus |
| `^` | Bitwise XOR |
| `&` | Bitwise AND |
| `\|` | Bitwise OR |
| `~` | Bitwise Negate |

### Logical Operators

| Operator | Description |
|----------|-------------|
| `&&` | And |
| `\|\|` | Or |
| `!` | Not |
| `==` | Equals |
| `!=` | Not equals |
| `>` / `>=` | Greater than / or equal |
| `<` / `<=` | Less than / or equal |

### Compound Assignment

```xscript
$count += 1;   $count -= 1;
$count *= 2;   $count /= 2;
++$count;      $count++;
--$count;      $count--;
```

### Constants

```xscript
$flag = RaceFlag::NPC;        // namespaced constant
$page = TextPage::MiscVoice;
$race = Xenon;                // top-level race constant
```

**Built-in constants:**

| Constant | Description |
|----------|-------------|
| `TRUE` / `FALSE` | Integer 1 / 0 |
| `NULL` | Null / no object |
| `PLAYERSHIP` | The player's current ship |
| `this` | The object this script is running on |
| `ThisSector` | The sector of `this` |
| `ThisOwner` | The race that owns `this` |
| `ThisHomebase` | The homebase of `this` |
| `ThisEnvironment` | The environment (sector/dock) of `this` |
| `DOCKEDAT` | The ship or station `this` is docked at |

---

### Function Main (v0.8)

`function main(...)` is the required script entry point, replacing the old `SetArgument()` pattern:

```xscript
#DESCRIPTION "My script"
#VERSION 1

function DATATYPE_NULL main(VARSECTOR $sector, VARSHIP $ship)
{
    // script body
    return(null);
}
```

The parameter list declares script arguments using pardef type names (`VARSECTOR`, `VARSHIP`, `NUMBER`, etc.). The optional return type after `function` can be a `DATATYPE_*` constant, `void` (equivalent to `DATATYPE_NULL`), or a pipe-separated combination (`DATATYPE_SHIP|DATATYPE_STATION`). If omitted, no return-type checking is performed.

---

### User-Defined Functions (v0.8)

Local-scoped functions define reusable routines with full variable isolation. All `$variables` inside a user function are private — the compiler mangles them automatically to `$var_fn.funcName` so they never collide with anything outside the function.

```xscript
function DATATYPE_INT clampValue(NUMBER $value, NUMBER $lo, NUMBER $hi)
{
    if ($value < $lo) { return($lo); }
    if ($value > $hi) { return($hi); }
    return($value);
}

function main(VARSECTOR $sector)
{
    $clamped = clampValue($count, 0, 100);
    return(null);
}
```

**Calling a user function:**

```xscript
$result = myFunction($arg1, $arg2);   // with return value
myFunction($arg1, $arg2);              // without return value
```

**Parameter types** can be any pardef keyword (`VARSECTOR`, `NUMBER`, `VALUE`, etc.) or a `DATATYPE_*` constant where an exact single-type pardef match exists. Omitting the type defaults to `VALUE`:

```xscript
function test($value1, $value2)   // both default to VALUE
{
    return($value1 + $value2);
}
```

**Return types** follow the same rules as `main`. Use `DATATYPE_NULL` or `void` for functions that don't return a value:

```xscript
function DATATYPE_NULL logSomething($msg)
{
    incomingMessage($msg, PlayerLog::Alert, TRUE);
}
```

**`return($x)` inside a user function** sets the function's return value and exits it. It does NOT exit the whole script (unlike `return` in `main`):

```xscript
function DATATYPE_INT checkSector(VARSECTOR $sector)
{
    if ($sector == NULL) { return(FALSE); }
    return(TRUE);
}
```

**Forward declarations** — functions can be called before they appear in the source file. The compiler performs a prepass scan to collect all signatures first.

**Recursion** — recursive calls produce a warning (not an error), since the shared mangled argument variables will be overwritten on each call.

**Name restrictions** — a user function name must not collide with an existing script command or constant (e.g. `function random()` is rejected).

---

### Subs (v0.8 Block Syntax)

Subs are top-level routines that share the global variable scope — they are the X3-native `label:`/`endsub;` mechanism wrapped in more readable syntax.

```xscript
sub initialise()
{
    $count = 0;
    $flag = TRUE;
}

function main(VARSECTOR $sector)
{
    gosub initialise;
    $count += 1;    // $count is accessible here — subs share global scope
    return(null);
}
```

Subs can appear before or after `main`. The compiler always places `main`'s compiled commands first in the output, followed by all subs, regardless of source order.

**Early exit** — `endsub;` inside a sub block returns to the caller:

```xscript
sub processShip()
{
    if ($ship == NULL) { endsub; }
    $ship->setCommand(null);
}
```

The classic `label:`/`endsub;` inline syntax remains valid and works identically.

**Key differences from user functions:**

| | User function | Sub |
|--|---------------|-----|
| Variable scope | Private (mangled) | Shared with global |
| Arguments | Declared in `(...)` | None |
| Return value | Yes, via `return($x)` | No |
| Call syntax | `$r = test($arg);` | `gosub test;` |
| `endsub` | Early exit only | Early exit or implicit at `}` |

---

### Labels, Goto and Gosub

Plain labels remain available for cases where block-syntax subs aren't appropriate:

```xscript
gosub doWork;
goto cleanup;

cleanup:
    $count = 0;

doWork:
    $workResult = 42;
endsub;
```

Label names must be unique across the entire script — no label, sub, or user function may share the same name.

---

### Global Functions

```xscript
incomingMessage("Hello", PlayerLog::Alert, TRUE);
$sector = getSectorByCoord(22, 3);
```

### Function Overloads

```xscript
$x = random(10);       // random(max)
$x = random(5, 10);    // random(min, max) — same name, different arity
```

### Namespaces

```xscript
$x    = Utils::random(10);
$flag = RaceFlag::NPC;
```

### Object Methods and Properties

```xscript
$exists = $ship->exists();
$name   = $ship->name;          // getter
$ship->name = "My Freighter";   // setter
```

### Nested Function Calls

```xscript
$name = getSectorByCoord(22, 3)->name;
incomingMessage($ship->getName(), PlayerLog::Alert, FALSE);
if getSectorByCoord(22, 3)->exists() { ... }
```

### Arrays and Tables

```xscript
$array[0] = 100;
$value = $grid[2][3];
$table["key"] = "value";
$count = arraySize($array);
$keys  = tableKeys($table);
$array[$i++] = 10;              // post-increment in subscript
```

### Conditions

```xscript
if ($count > 10) { ... }
else if ($count > 5) { ... }
else { ... }

if not $ship->exists() { ... }

while ($count < 10)
{
    $count += 1;
}
```

### While Loop — Increment in Condition

```xscript
while ($i++ < 10) { ... }
while (arraySize($myArray) < 10) { ... }
```

### Break and Continue

```xscript
while ($i < 100)
{
    if ($i == 50) { break; }
    if ($i == 25) { continue; }
    $i += 1;
}
```

### START Modifier

```xscript
START incomingMessage("Hello", PlayerLog::Alert, TRUE);
START $ship->call("plugin.myscript");
```

---

## Preprocessor

### Script Metadata

```xscript
#DESCRIPTION "My plugin script"
#VERSION 42
#COMMAND 1234
```

These appear before the `function main(...)` declaration.

### Datatype Hints

```xscript
#datatype $wing DATATYPE_WING
#datatype $obj DATATYPE_SHIP|DATATYPE_STATION
```

### Defines

```xscript
#define MAX_COUNT 100
#define ADD(a, b) a + b
#define DEBUG
#undef DEBUG
```

**Multi-line:**

```xscript
#define BIG_VALUE \
    100 + 200 + 300
```

### Conditional Compilation

```xscript
#ifdef PLATFORM_PC
    $platform = 1;
#elseif PLATFORM_LINUX
    $platform = 2;
#else
    $platform = 0;
#endif

#ifdef VERSION >= 1
    // compiled when VERSION is defined and equals 1 or higher
#endif

#ifndef DEBUG
    $logLevel = 0;
#endif
```

Supported comparison operators in conditions: `==`, `!=`, `>`, `<`, `>=`, `<=`

**Command-line defines:**

```
XScriptCompiler.exe --load_data data.dat --compile script.xs --out script.xml --define:DEBUG
```

### Include Files

```xscript
#include "utils.xs"
#include "common/helpers.xs"
```

Paths are resolved relative to the including file. Circular includes are detected and reported as an error.

---

## Function Macros

```xscript
foreach($item, $myArray)
{
    incomingMessage($item->name, PlayerLog::Alert, TRUE);
}
```

Macros expand at compile time to native XScript. Additional macros may be defined in `x3fl.xml` under `<Macros>`.

---

## Type Checking

The compiler tracks object types through assignments and function return values. Methods are validated against the inferred type of the object. User function parameters declared with a specific pardef are checked at call sites. Type mismatches produce warnings rather than errors.

---

## Data File

**Priority order when searching for the data file:**
1. Path specified via `--load_data`
2. `x3fl.dat` in the working directory
3. `default_data.dat` in the working directory

---

## Exit Codes

| Code | Meaning |
|------|---------|
| `0` | Compilation successful |
| `1` | Compilation failed (errors reported to stdout) |

---

## VS Code Extension

A Visual Studio Code extension (`xscript-x3fl-extension.zip`) provides editor support for `.xs` files.

**Features:**
- Syntax highlighting — keywords (`function`, `sub`, `endsub`, `void`), function/sub definition headers, pardef return types, variables, operators, strings, preprocessor directives
- Snippet completions — `function main`, `function`, `sub`, `if`, `while` structural templates with tab stops
- IntelliSense — autocomplete for all ~2,500 functions, methods, properties, constants, namespaces, and macros
- Signature help and hover documentation
- Compiler errors/warnings appear as squiggles in the editor and in the Problems panel
- Compile from the editor title bar, right-click menu, or `Ctrl+Shift+B`; optional compile-on-save

**Configuration:**

| Setting | Description |
|---------|-------------|
| `xscript.compiler.exePath` | Full path to `XScriptCompiler.exe` |
| `xscript.compiler.dataFile` | Path to `default_data.dat` (auto-detected if in workspace root) |
| `xscript.compiler.outputDir` | Where compiled `.xml` files are written (defaults to alongside the `.xs` file) |
| `xscript.compiler.compileOnSave` | Set to `true` to compile automatically on every save |
| `xscript.compiler.defines` | Array of symbols to pre-define, e.g. `["DEBUG", "PLATFORM_PC"]` |

Place `default_data.dat` (or `x3fl.dat`) in your workspace root and the extension loads function definitions automatically on startup.

---

## Version History

**0.8 BETA** — Current release
- **`function main(...)` wrapper** — all scripts wrap their body in a `function main(...)` block; arguments declared as typed parameters; `#DESCRIPTION`, `#VERSION`, `#COMMAND` replace the equivalent runtime calls; `return(value)` at end of main
- **User-defined local-scoped functions** — `function name(params) { ... }` with full variable isolation via automatic name mangling (`$var` → `$var_fn.name`); `return($x)` exits the function; forward declarations via prepass scan; recursion warning; name-collision checking against existing commands
- **`sub name() { ... }` block syntax** — readable wrapper for native label/gosub/endsub; subs share the global variable scope; `endsub;` for early exit; correct output ordering regardless of source position relative to `main`
- **Pardef-free parameters** — `function test($var)` defaults untyped parameters to `VALUE`
- **`DATATYPE_*` constants in parameter lists** — accepted where an exact single-type pardef match exists
- **Duplicate label/sub/function detection** — clear errors for name collisions across labels, subs, and user functions
- **Decompiler updated** — output uses `function main(...)` syntax; argument descriptions preserved as comments; `#VERSION`/`#DESCRIPTION`/`#COMMAND` directives emitted before the function
- **VS Code extension v1.4.0** — `function`/`sub`/`endsub`/`void` keyword highlighting; function and sub definition header patterns; structural snippet completions

**0.7 BETA**
- Double (chained) assignment
- Function overloads/aliases — multiple functions sharing a name, resolved by argument count
- Namespaces — `Utils::random(...)`, namespaced constants
- Function macros — `foreach($value, $array) { ... }` and custom macros defined in `x3fl.xml`
- `#datatype` preprocessor directive
- Multi-line `#define` with `\` continuation
- VS Code extension v1.3.0

**0.6 BETA**
- While loop condition re-evaluation with `++`/`--` and function calls
- `break` and `continue` inside while loops
- Nested function calls as arguments and in conditions
- Post-increment in array subscripts
- Preprocessor system — `#define`, `#ifdef`/`#ifndef`/`#elseif`/`#else`/`#endif`, `#include`
- Script metadata — `#DESCRIPTION`, `#VERSION`, `#COMMAND`
- Command-line `--define:NAME` flag

**0.5 BETA**
- Full XScript language — arrays, tables, compound assignment, namespace constants, object methods/properties
- Two-pass compilation for correct type tracking across gosub/label boundaries
- Object type propagation through assignments and function return values
- DataType prefix support for mod-added commands (`SHIPCOMMAND_1000`)
