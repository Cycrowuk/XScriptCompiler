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

### Variables

Variables are prefixed with `$`. Dot notation is supported for sub-variables.

```xscript
$myVar = 42;
$my.variable = "hello";
$result = getSectorByCoord(22, 3);
```

### Object Methods and Properties

Object methods and properties both use the `->` operator.

**Methods** call a function on the object and optionally return a value:

```xscript
$name   = $ship->getName();
$exists = $ship->exists();
$ship->setCommand(null);
```

**Properties** provide a cleaner syntax for getting and setting values without explicit function call brackets. Each property maps to an underlying getter function (for reads) and optionally a setter function (for writes).

Reading a property (getter):

```xscript
$name   = $ship->name;
$speed  = $ship->maxSpeed;
$sector = $ship->sector;
```

Writing a property (setter):

```xscript
$ship->name = "My Freighter";
$ship->commander = PLAYERSHIP;
```

Properties can also be used directly in conditions:

```xscript
if $ship->isPlayer
{
    speak($pilot, "That is the player", 1000);
}

do if $ship->exists;
```

Read-only properties have no setter — attempting to assign to them produces a compile error:

```xscript
$ship->race = 1;    // error if 'race' has no setter defined
```

Methods and properties can be chained when the return type is an object:

```xscript
$pilotName = $ship->pilot->name;
$pilotSector = PLAYERSHIP->sector->name;
```

### Nested Function Calls

In standard X3 scripts, function return values must always be assigned to a variable before being used elsewhere. XScript removes this restriction — functions can be used directly as arguments to other functions, and the compiler generates the necessary temporary variables automatically.

```xscript
// Standard X3: requires intermediate variable
$sector = getSectorByCoord(22, 3);
$name = $sector->name;

// XScript: function call directly as argument
$name = getSectorByCoord(22, 3)->name;
```

```xscript
// Passing a function's return value as an argument to another function
speak($pilot, $ship->getName(), 1000);

// Nested object method calls
$pilotName = PLAYERSHIP->pilot->name;

// Multiple levels of nesting
if getSectorByCoord(22, 3)->exists()
{
    // ...
}
```

This applies to global functions, object methods, and properties — any combination can be nested:

```xscript
$result = getObjectByName(this->sector, "My Station")->cargo[0];
```

The compiler handles all temporary variable allocation internally — the generated script is fully compatible with the X3 engine.

### Conditions

```xscript
if ($value > 100)
{
    $result = "high";
}
else if ($value > 50)
{
    $result = "medium";
}
else
{
    $result = "low";
}

while ($count < 10)
{
    $count += 1;
}

do if $ship->exists();

skip if $value == 0;
```

The `not` keyword inverts any condition:

```xscript
if not $ship->exists()
{
    return null;
}
```

### Return Values

Functions with return values use the assignment form:

```xscript
$sector = getSectorByCoord(22, 3);
```

Conditional return — the function's result is used directly as the condition:

```xscript
if $ship->exists()
{
    speak($pilot, "Ship found", 1000);
}
```

Returning from a script:

```xscript
return $result;
return null;
```

### START Modifier

`START` runs a function asynchronously in a separate thread without waiting for a return value:

```xscript
START speak($pilot, "Hello", 1000);
START $ship->call("plugin.myscript");
```

### Constants

**Boolean and null:**

```xscript
$exists = TRUE;
$empty  = FALSE;
$obj    = NULL;
```

**Script context — the object the script is running on:**

The `this` constant refers to the object that owns the currently running script (the ship, station, or other entity it is attached to). Several related constants provide convenient access to its context:

```xscript
$self    = this;              // the object this script runs on
$home    = ThisHomebase;      // this object's homebase
$env     = ThisEnvironment;   // docking bay or sector this object is in
$sector  = ThisSector;        // sector this object is in
$owner   = ThisOwner;         // race that owns this object
$docked  = DOCKEDAT;          // environment this object is docked at
$true    = TRUEOWNER;         // true race owner of this object
```

These can be used directly with `->` to call methods on the owning object:

```xscript
$name = this->name;
this->setCommand(null);
START this->call("plugin.myscript");
```

**Other built-in constants:**

```xscript
$player = PLAYERSHIP;         // the player's ship
```

Namespace constants use `::` syntax:

```xscript
$flag = RaceFlag::NPC;
$type = ShipType::M3;
```

Custom mod commands use a prefix followed by the command ID:

```xscript
$cmd = SHIPCOMMAND_1000;
```

### Arrays and Tables

XScript supports two types of indexed collections: **arrays** (integer-indexed) and **tables** (any-type keyed).

**Arrays** use integer indices starting at 0:

```xscript
$value    = $array[0];
$array[0] = 42;
$array[1] = "hello";

// Multi-dimensional
$value      = $grid[2][3];
$grid[0][1] = 100;
```

**Tables** use any datatype as the key — strings, integers, objects, or constants:

```xscript
$value             = $table["key"];
$table["name"]     = "My Ship";
$table[42]         = "answer";
$table[PLAYERSHIP] = "player";
$table[RaceFlag::NPC] = TRUE;
```

Both can be used directly in conditions:

```xscript
if ($array[0])
{
    // ...
}

if ($table["active"])
{
    // ...
}
```

Function return values can be used as the index expression:

```xscript
$value = $array[$ship->cargoCount];
$data  = $table[$ship->name];
```

**Utility functions:**

```xscript
$count = arraySize($array);      // number of entries in an array
$keys  = tableKeys($table);      // returns an array of all keys in a table
```

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

### Labels, Goto and Gosub

`gosub` jumps to a label and returns when `endsub` is reached. Variables assigned inside a sub are visible to the caller.

```xscript
gosub initialise;

$result = $myVar->exists();    // $myVar is known here, assigned in the sub below
return null;

initialise:
    $myVar = PLAYERSHIP;
endsub;
```

`goto` is a one-way jump with no return:

```xscript
goto cleanup;

cleanup:
    $ship->setCommand(null);
```

### Defines (Macros)

```xscript
#define MAX_COUNT 100
#define ADD(a, b) a + b

$limit = MAX_COUNT;
$sum   = ADD($x, $y);
```

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

## Version History

**0.5 BETA** — Current release
- Full XScript language support including arrays, compound assignment, namespace constants, and custom prefix constants
- Two-pass compilation for correct type tracking across gosub/label boundaries
- Object type propagation through assignments and function return values
- Boolean arguments correctly emit `TRUE`/`FALSE` in decompiled output
- DataType prefix support for mod-added commands (`SHIPCOMMAND_1000`)
- Improved error messages with source line and caret indicator
