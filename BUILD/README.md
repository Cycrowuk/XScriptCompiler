# XScript Compiler
### Version 0.7 BETA
#### X3 Farnham's Legacy Script Compiler

Website: https://www.xpluginmanager.co.uk/xscript/

---

## Overview

XScript Compiler translates human-readable XScript source files (`.xs`) into the XML script format used by X3 Farnham's Legacy. It provides full type checking, meaningful error messages with source location, and supports the complete XScript language including object methods, conditions, arrays, labels, and mod-added custom commands.

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

The build process reads several files from the X3 Farnham's Legacy game data directory. These must be present in a `Data\` subdirectory relative to the working directory when the compiler is run.

| File | Description |
|------|-------------|
| `Data\0001-L044.xml` | Game text file — provides display names and descriptions for object commands and ware types. The language and text page prefixes are configured in `x3fl.xml` under `<GameData>`. |
| `Data\<name>.txt` | Ware type files — one per `<WareType>` entry in `x3fl.xml`. Each file contains the list of ware identifiers (ship types, station types, missile types, etc.) for that category. The filename for each type is specified via the `file` attribute in the `<WareType>` definition. |

A typical directory layout for running `--builddata`:

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
| `--usenamespace` | Emit namespaced function calls (e.g. `Utils::random`) instead of the plain global name, where a namespace mapping exists |

The decompiler resolves command IDs back to their function names, constants back to their symbolic names (e.g. `TRUE`/`FALSE`, `PLAYERSHIP`, `RaceFlag::NPC`), and boolean arguments to `TRUE`/`FALSE`. Commands from third-party mods that use a DataType prefix (e.g. `SHIPCOMMAND_1000`) are emitted using the prefix notation rather than raw integers.

---

## Error Output

Errors and warnings are written to stdout in the following format:

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
   anything between these is ignored
   by the compiler */
```

### Variables

Variables begin with `$`. The character immediately after `$` must be a letter. Subsequent characters may be letters, numbers, underscores, or periods (`.`):

```xscript
$count = 0;
$my.variable = "hello";
$ship2 = PLAYERSHIP;
```

### Numbers

X3 scripts only support **integer** values — no floating point. A number literal may only contain the digits `0` through `9`:

```xscript
$x = 42;
$y = -10;
```

### Assignments

An assignment uses a single `=` and must have a variable or array on the left:

```xscript
$variable = 10;
$array[1] = 10;
```

**Chained (double) assignment** — assigning the same value to two variables in one statement:

```xscript
$x = $y = -1;
$x = $y = random(2);
```

The compiler expands this so `$y` is assigned first, then `$x` is assigned the value of `$y`.

### Expression Operators

Most operators are only valid with integer values. For strings, only `+` (concatenation) is valid:

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

For use in comparisons and conditional expressions:

| Operator | Description |
|----------|-------------|
| `&&` | And |
| `\|\|` | Or |
| `!` | Not |
| `==` | Equals |
| `!=` | Not equals |
| `>` | Greater than |
| `>=` | Greater than or equals |
| `<` | Less than |
| `<=` | Less than or equals |

### Compound Assignment

```xscript
$count += 1;
$count -= 1;
$count *= 2;
$count /= 2;
++$count;
$count++;
--$count;
$count--;
```

### Constants

Named constants use the `::` namespace separator:

```xscript
$flag = RaceFlag::NPC;
$page = TextPage::MiscVoice;
$type = ShipType::M3;
```

Race constants are top-level (no namespace):

```xscript
$race = Xenon;
$race = Argon;
```

**Built-in constants:**

| Constant | Description |
|----------|-------------|
| `TRUE` | Integer value 1 |
| `FALSE` | Integer value 0 |
| `NULL` | Null / no object |
| `PLAYERSHIP` | The ship the player is currently piloting |

**Script context constants** — refer to the object the script is attached to. These are `null` when the script runs globally:

| Constant | Description |
|----------|-------------|
| `this` | The object this script is running on |
| `ThisHomebase` | The homebase of `this` |
| `ThisEnvironment` | The environment of `this` — the sector or docking ship/station it is in |
| `ThisSector` | The sector of `this` |
| `ThisOwner` | The race that owns `this` |
| `DOCKEDAT` | The ship or station `this` is currently docked at |
| `TRUEOWNER` | The true owner race — relevant when an object is disguising its race (e.g. pirates) |

```xscript
$name   = this->name;
$sector = ThisSector;
START this->call("plugin.myscript");
```

**Custom prefix constants** — for mod-added commands not in the data file:

```xscript
$cmd = SHIPCOMMAND_1000;
```

### Global Functions

Functions translate to X3 script commands. Arguments are comma-separated inside parentheses:

```xscript
incomingMessage("Hello", PlayerLog::Alert, TRUE);
$sector = getSectorByCoord(22, 3);
```

### Function Overloads

Some functions have multiple variants with different argument counts. These can share a single name — the compiler picks the best match based on the number (and type) of arguments supplied:

```xscript
$x = random(10);       // random(max) — 1 argument
$x = random(5, 10);     // randomFrom(min, max) — 2 arguments, same name
```

Where an argument can be either a literal script-name string or a variable, the compiler also disambiguates by argument type — a string literal prefers the `CALLNAME` variant, while a variable prefers the general variant:

```xscript
registerHotkey("plugin.myscript");  // string literal — CALLNAME variant
registerHotkey($scriptName);        // variable — general variant
```

### Namespaces

Functions and constants can be organised under a namespace using `::`, purely for organisation and autocomplete — `Namespace::name` resolves to the underlying global function or constant:

```xscript
$x = Utils::random(10);        // same as random(10)
$flag = RaceFlag::NPC;          // constant namespace
```

### Object Methods and Properties

Object methods and properties use the `->` operator.

**Methods** call a function on the object:

```xscript
$exists = $ship->exists();
$ship->setCommand(null);
```

**Properties** map to getter and setter functions with cleaner syntax — no parentheses needed:

```xscript
// Reading (getter)
$name   = $ship->name;
$sector = $ship->sector;

// Writing (setter)
$ship->name      = "My Freighter";
$ship->commander = PLAYERSHIP;

// In conditions
if $ship->isPlayer
{
    incomingMessage("That is the player", PlayerLog::Alert, TRUE);
}
```

Read-only properties have no setter — assigning to them produces a compile error.

### Nested Function Calls

Unlike standard X3 scripts, XScript allows function return values to be used directly as arguments or chained with `->` without first assigning to a variable. The compiler generates the necessary temporary variables automatically:

```xscript
// Standard X3 requires an intermediate variable:
$sector = getSectorByCoord(22, 3);
$name = $sector->name;

// XScript: direct nesting
$name = getSectorByCoord(22, 3)->name;

// As an argument to another function
incomingMessage($ship->getName(), PlayerLog::Alert, FALSE);

// In a condition
if getSectorByCoord(22, 3)->exists()
{
    // ...
}
```

This applies to global functions, object methods, and properties in any combination. The generated script is fully compatible with the X3 engine.

### Arrays and Tables

Both use subscript operators `[` and `]`. The subscript can be a value, variable, expression, or function call.

**Arrays** are integer-indexed. A warning is shown if a non-integer subscript is used on an array:

```xscript
$value      = $array[0];
$array[1]   = 42;

// Multi-dimensional
$value      = $grid[2][3];
$grid[0][1] = 100;
```

**Tables** accept any datatype as a key — strings, integers, objects, or constants:

```xscript
$value             = $table["key"];
$table["name"]     = "My Ship";
$table[42]         = "answer";
$table[PLAYERSHIP] = "player";
$table[RaceFlag::NPC] = TRUE;
```

Arrays and tables can appear in any expression, condition, or function argument. The subscript operator can also be applied directly to a function return value if it returns an array or table type:

```xscript
if ($array[$ship->cargoCount])
{
    // ...
}
```

Post-increment in array subscripts works correctly — the current value is used as the index and the increment fires after the operation:

```xscript
$array[$i++] = 10;   // uses $i as index, then increments $i
$array[$i++] = 20;   // uses the incremented $i
```

**Utility functions:**

```xscript
$count = arraySize($array);   // number of entries in an array
$keys  = tableKeys($table);   // returns an array of all keys in a table
```

### Conditions

Condition keywords: `if`, `else`, `not`, `while`

Keywords can be combined:

```xscript
if ($value > 100) { ... }
else if ($value > 50) { ... }
else { ... }

if not $ship->exists() { ... }
else if not ($count == 0) { ... }

while ($count < 10)
{
    $count += 1;
}
```

Unlike standard X3, `do if` and `skip if` do not exist as explicit keywords — the compiler automatically selects the appropriate X3 instruction based on the size of the block.

### While Loop — Increment in Condition (v0.6)

`inc` and `dec` functions, and `++`/`--` operators, can appear directly in a while condition. The compiler automatically re-evaluates the condition expression before each `continue` and at the end of the loop body so the loop behaves correctly:

```xscript
// Post-increment: uses current $i, then increments
while ($i++ < 10)
{
    // $i has been incremented before reaching here
}

// Pre-increment: increments $i first, then tests
while (++$i < 10)
{
    // $i is already incremented
}

// Function in condition: re-evaluated each iteration
while (arraySize($myArray) < 10)
{
    $myArray[$i] = $i;
    $i += 1;
}
```

### Blocks

Braces `{` `}` group multiple statements under a conditional. A single-statement condition does not require braces:

```xscript
if ($x > 0)
{
    $a = 1;
    $b = 2;
}

if ($x > 0)
    $a = 1;
```

### Break and Continue

`break` exits the nearest enclosing `while` loop immediately. `continue` jumps to the next iteration:

```xscript
while ($i < 100)
{
    if ($i == 50) break;
    if ($i == 25) continue;
    $i += 1;
}
```

### Return Values

```xscript
return $result;
return null;
```

A function's return value can be used directly as a condition:

```xscript
if $ship->exists()
{
    incomingMessage("Ship found", PlayerLog::Alert, TRUE);
}
```

### START Modifier

Runs a function asynchronously without waiting for a return value:

```xscript
START incomingMessage("Hello", PlayerLog::Alert, TRUE);
START $ship->call("plugin.myscript");
START this->call("plugin.myscript");
```

### Labels, Goto and Gosub

**`goto`** is a one-way jump to a label:

```xscript
goto cleanup;

cleanup:
    $ship->setCommand(null);
```

**`gosub`** jumps to a label and returns when `endsub` is reached. Variables assigned inside a sub are visible to the caller — the compiler pre-scans the sub before compiling the calling code:

```xscript
gosub initialise;

$result = $myVar->exists();    // $myVar known here — assigned in the sub below
return null;

initialise:
    $myVar = PLAYERSHIP;
endsub;
```

---

## Preprocessor

The preprocessor runs before compilation and processes directives that begin with `#`. Preprocessor lines are never passed to the compiler — they control which code is compiled and set script metadata.

### Script Metadata

Set the script description, version, and command ID directly in the source file:

```xscript
#DESCRIPTION "My plugin script"
#VERSION 42
#COMMAND 1234
```

These are equivalent to calling `setDescription`, `setVersion`, and `setCommand` in the body of the script, but placing them at the top makes the intent clear. `#COMMAND` also accepts a named object command constant.

### Datatype Hints

`#datatype` tells the compiler the type(s) a variable will hold, so object methods and properties can be validated even when the compiler cannot infer the type from an assignment (e.g. a variable received as a script argument):

```xscript
#datatype $wing DATATYPE_WING
#datatype $obj DATATYPE_SHIP|DATATYPE_STATION
```

Multiple types are separated by `|`. This is a hint only — it does not generate any script output, it simply informs the type checker.

### Defines

```xscript
#define MAX_COUNT 100
#define ADD(a, b) a + b
#define CLAMP(v, lo, hi) (v < lo) ? lo : (v > hi) ? hi : v

$limit = MAX_COUNT;
$sum   = ADD($x, $y);
```

A define with no replacement value registers the symbol for `#ifdef` use:

```xscript
#define DEBUG
```

Remove a previously defined symbol:

```xscript
#undef DEBUG
```

**Multi-line defines** — a trailing `\` continues the define onto the next line:

```xscript
#define BIG_VALUE \
    100 + 200 + 300

#define COMPLEX_CHECK(a, b) \
    (a > 0) && \
    (b > 0)
```

### Conditional Compilation

```xscript
#define PLATFORM_PC

#ifdef PLATFORM_PC
$platform = 1;
#elseif PLATFORM_LINUX
$platform = 2;
#else
$platform = 0;
#endif

#ifndef DEBUG
// Only compiled in release builds
$logLevel = 0;
#endif
```

**Comparison conditions** — test the value of a define, not just its presence:

```xscript
#define VERSION 2

#ifdef VERSION == 2
// compiled only when VERSION is defined and equals 2
#endif

#ifdef VERSION >= 1
// compiled when VERSION is 1 or higher
#endif
```

Supported comparison operators: `==`, `!=`, `>`, `<`, `>=`, `<=`

**Nesting** is fully supported:

```xscript
#ifdef FEATURE_A
    #ifdef FEATURE_B
    // only when both are defined
    #endif
#endif
```

**Command-line defines** — pass `--define:NAME` when compiling to inject a symbol without modifying the source file:

```
XScriptCompiler.exe --load_data data.dat --compile script.xs --out script.xml --define:DEBUG --define:PLATFORM_PC
```

### Include Files

```xscript
#include "utils.xs"
#include "common/helpers.xs"
```

The included file is processed inline at the point of the `#include` directive, as if its contents were written directly into the source. Paths are resolved relative to the including file's directory. Nested includes are supported. Circular includes are detected and reported as an error.

All preprocessor state (`#define`, `#ifdef` depth) is shared between the including file and the included file — defines set in an included file are visible in the including file after the `#include` line, and `#ifdef`/`#endif` blocks may span include boundaries.

---

## Function Macros

X3 has no native `for`/`foreach` construct — only `while`. The compiler provides language-level macros, defined in the data file, that expand to native XScript at compile time. The built-in `foreach` macro is one example:

```xscript
foreach($item, $myArray)
{
    incomingMessage($item->name, PlayerLog::Alert, TRUE);
}
```

This expands into the equivalent `while` loop with a counter, array lookup, and increment — written exactly as if you had typed it yourself. The generated script contains no trace of the macro; it is a pure compile-time expansion.

A single-statement body (no braces) is also supported:

```xscript
foreach($item, $myArray)
    incomingMessage($item->name, PlayerLog::Alert, TRUE);
```

Warnings about a macro's arguments — for example, if `$myArray` is uninitialised or not an array — are reported against the `foreach(...)` call itself, not the internal expanded code.

Additional macros may be defined in `x3fl.xml` under `<Macros>`, following the same `<Block>`/`<BlockCommands/>`/`%ARGn%` substitution pattern as `foreach`.

---

## Type Checking

The compiler tracks object types through assignments and function return values. Methods are validated against the declared type of the object.

```xscript
$sector = getSectorByCoord(22, 3);   // $sector tracked as DATATYPE_SECTOR
$name   = $sector->getName();        // validated: getName() exists on sectors
```

Type mismatches produce warnings rather than errors, since X3 script execution is dynamic.

---

## Known Limitations (Beta)

- Multi-line string literal continuation with `\` is not yet implemented (multi-line `#define` is supported)
- `goto` targets are not validated until `finalise` — forward `goto` references will compile but may produce runtime errors if the label does not exist

---

## Data File

The compiler requires a data file containing all game function definitions, constants, and datatypes.

**Priority order when searching for the data file:**
1. Path specified via `--load_data`
2. `x3fl.dat` in the working directory
3. `default_data.dat` in the working directory

The data file is compiled from `x3fl.xml` using the data compiler tool. The binary `.dat` format loads significantly faster than the XML source and should be preferred for regular use.

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
- Syntax highlighting for all XScript keywords, operators, constants, variables, and preprocessor directives (including `#datatype` and multi-line `#define`)
- IntelliSense — autocomplete for functions, object methods, properties, constants, namespaces (`Utils::`), and function macros (`foreach`), with parameter hints and documentation from the definition file
- Hover documentation — hover over any function, constant, namespace member, or macro to see its description, parameters, and return type
- Error and warning squiggles — compiler errors appear as red/yellow underlines directly in the editor, listed in the Problems panel (`Ctrl+Shift+M`)
- Compile from the editor — click the play button in the editor title bar, or use the right-click context menu
- Compile on save — optionally compile automatically whenever a `.xs` file is saved

**Installation:**

1. Extract `xscript-x3fl-extension.zip`
2. Open VS Code
3. Press `Ctrl+Shift+X` to open the Extensions panel
4. Click the `…` menu and choose **Install from VSIX…**
5. Select the `.vsix` file, or run `vsce package` inside the extracted folder first

**Configuration:**

Open Settings (`Ctrl+,`) and search for `xscript` to configure:

| Setting | Description |
|---------|-------------|
| `xscript.compiler.exePath` | Full path to `XScriptCompiler.exe` |
| `xscript.compiler.dataFile` | Path to `default_data.dat` (auto-detected if in workspace root) |
| `xscript.compiler.outputDir` | Where compiled `.xml` files are written (defaults to same folder as the `.xs` file) |
| `xscript.compiler.compileOnSave` | Set to `true` to compile automatically on every save |
| `xscript.compiler.defines` | Array of symbols to pre-define, e.g. `["DEBUG", "PLATFORM_PC"]` |

Place `default_data.dat` (or `x3fl.dat`) in your workspace root folder and the extension will load function definitions and constants automatically on startup.

---

## Version History

**0.7 BETA** — Current release
- Double (chained) assignment — `$x = $y = -1;` and `$x = $y = random(2);`
- Function overloads/aliases — multiple functions sharing a name, resolved by argument count, with `CALLNAME` vs string-literal disambiguation (e.g. `registerHotkey`)
- Namespaces — `Utils::random(...)` resolves to the underlying global function; namespaced constants (e.g. `RaceFlag::NPC`) supported alongside namespaced functions
- Function macros — language-level constructs (e.g. `foreach($value, $array) { ... }`) that expand to native `while` loops at compile time; defined in the data file via `<Macros>`
- `#datatype` preprocessor directive — hints a variable's type(s) for the type checker (`#datatype $wing DATATYPE_WING`)
- Multi-line `#define` — trailing `\` continues a define onto subsequent lines
- Fixed nested `if` blocks producing incorrect consecutive closing braces
- Fixed `$x - 2` being misparsed as negation; `random(-1)` and other negative-literal arguments now compile correctly
- VS Code extension updated to v1.3.0 — namespace/macro autocomplete, hover, signature help; `#datatype` and multi-line `#define` syntax highlighting

**0.6 BETA** — Previous release
- While loop condition re-evaluation — `++`/`--` and function calls in `while(...)` conditions re-evaluate correctly on each iteration
- `break` and `continue` inside while loops
- Nested function calls as arguments — `random($i + 10)`, `if(inc($count))`, etc.
- `inc`/`dec` with expression arguments — `inc($i + 10)` compiles to `$i = $i + 10`
- Post-increment in array subscripts — `$array[$i++]` uses then increments
- Mixed increment/decrement in single expressions — `$array[$i++] = --$j`
- `else if` with function calls in condition — temp vars correctly hoisted before the whole if/else chain
- Preprocessor system — `#define`, `#undef`, `#ifdef`, `#ifndef`, `#elseif`, `#elseifdef`, `#elseifndef`, `#else`, `#endif` with value comparisons and nesting
- Script metadata directives — `#DESCRIPTION`, `#VERSION`, `#COMMAND`
- `#include` — inline file inclusion with circular include detection
- Command-line defines — `--define:NAME` flag (multiple supported)
- Uninitialised variable warnings now fire for variables inside function expression arguments
- Compiler comment in compiled scripts identifies the XScript compiler version and website

**0.5 BETA** — Previous release
- Full XScript language support including arrays, compound assignment, namespace constants, and custom prefix constants
- Two-pass compilation for correct type tracking across gosub/label boundaries
- Object type propagation through assignments and function return values
- Boolean arguments correctly emit `TRUE`/`FALSE` in decompiled output
- DataType prefix support for mod-added commands (`SHIPCOMMAND_1000`)
- Improved error messages with source line and caret indicator
