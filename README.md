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
```

**Arguments:**

| Argument | Description |
|----------|-------------|
| `--load_data <file>` | Path to the game data file (`default_data.dat` or `x3fl.dat`) |
| `--compile <file>` | Path to the XScript source file to compile |
| `--out <file>` | Path for the compiled XML output file |

**Example:**

```
XScriptCompiler.exe --load_data default_data.dat --compile myscript.xs --out myscript.xml
```

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

Object methods use the `->` operator. Properties use the same syntax.

```xscript
$ship = PLAYERSHIP;
$name = $ship->getName();
$exists = $ship->exists();
$ship->setName("My Ship");
```

### Global Functions

```xscript
$sector = getSectorByCoord(22, 3);
speak($pilot, $text, 1000);
```

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

```xscript
$exists = TRUE;
$empty  = FALSE;
$obj    = NULL;
$player = PLAYERSHIP;
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

### Arrays

```xscript
$value = $array[0];
$array[0] = 42;

// Multi-dimensional
$value = $grid[2][3];
```

Arrays can be used directly in conditions:

```xscript
if ($array[0])
{
    // ...
}
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
