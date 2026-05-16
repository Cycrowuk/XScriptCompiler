# XScript Compiler
### Version 0.5 BETA
#### X3 Farnham's Legacy Script Compiler

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
$page = TextPage::Menus;
$type = ShipType::M3;
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

### Defines (Macros)

```xscript
#define MAX_COUNT 100
#define ADD(a, b) a + b

$limit = MAX_COUNT;
$sum   = ADD($x, $y);
```


## Type Checking

The compiler tracks object types through assignments and function return values. Methods are validated against the declared type of the object.

```xscript
$sector = getSectorByCoord(22, 3);   // $sector tracked as DATATYPE_SECTOR
$name   = $sector->getName();        // validated: getName() exists on sectors
```

Type mismatches produce warnings rather than errors, since X3 script execution is dynamic.

---

## Known Limitations (Beta)

- Double assignment (`$a = $b = func()`) is not yet supported
- Multi-line string continuation with `\` is not yet implemented
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

A Visual Studio Code extension is included (`xscript-x3fl.vsix`) providing editor support for `.xs` files.

**Features:**
- Syntax highlighting for all XScript keywords, operators, constants, and variables
- IntelliSense — autocomplete for functions, object methods, properties, and constants, with parameter hints and documentation from the definition file
- Hover documentation — hover over any function or constant to see its description, parameters, and return type
- Error and warning squiggles — compiler errors appear as red/yellow underlines directly in the editor, listed in the Problems panel (`Ctrl+Shift+M`)
- Compile from the editor — `Ctrl+Shift+B` compiles the active file using your configured compiler settings

**Installation:**

1. Open VS Code
2. Press `Ctrl+Shift+X` to open the Extensions panel
3. Click the `…` menu and choose **Install from VSIX…**
4. Select the `xscript-x3fl.vsix` file

**Configuration:**

Open Settings (`Ctrl+,`) and search for `xscript` to configure:

| Setting | Description |
|---------|-------------|
| `xscript.compiler.exePath` | Full path to `XScriptCompiler.exe` |
| `xscript.compiler.dataFile` | Path to `default_data.dat` (auto-detected if in workspace root) |
| `xscript.compiler.outputDir` | Where compiled `.xml` files are written (defaults to same folder as the `.xs` file) |
| `xscript.compiler.compileOnSave` | Set to `true` to compile automatically on every save |

Place `default_data.dat` (or `x3fl.dat`) in your workspace root folder and the extension will load function definitions and constants automatically on startup.

---

## Version History

**0.5 BETA** — Current release
- Full XScript language support including arrays, compound assignment, namespace constants, and custom prefix constants
- Two-pass compilation for correct type tracking across gosub/label boundaries
- Object type propagation through assignments and function return values
- Boolean arguments correctly emit `TRUE`/`FALSE` in decompiled output
- DataType prefix support for mod-added commands (`SHIPCOMMAND_1000`)
- Improved error messages with source line and caret indicator
