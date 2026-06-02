// ============================================================
//  XScript Compiler Feature Test
//  Tests all language features and 0.6 compiler improvements
// ============================================================

// ── Basic variables and assignment ───────────────────────────
$count = 0;
$name = "test ship";
$flag = TRUE;
$empty = NULL;

// ── Constants ────────────────────────────────────────────────
$player = PLAYERSHIP;
$sector = ThisSector;
$owner  = ThisOwner;

// ── Namespace constants ───────────────────────────────────────
$raceFlag = Xenon;
$page     = TextPage::Menus;

// ── Simple if/else if/else ────────────────────────────────────
if ($count > 10)
{
    $count = 0;
}
else if ($count > 5)
{
    $count = 5;
}
else
{
    $count = 1;
}


// ── Nested functions as arguments (0.6) ──────────────────────
$result = random($count + 10);

// ── if with function in condition ────────────────────────────
if (random($count + 5) > 3)
{
    $count += 1;
}
else if (random($count + 10) > 6)
{
    $count += 2;
}
else if (random($count + 20))
{
    $count += 3;
}

// ── inc/dec in condition ─────────────────────────────────────
if (inc($count))
{
    $result = $count;
}

// ── inc with expression ───────────────────────────────────────
inc($count + 5);

// ── Compound assignment ───────────────────────────────────────
$count += 10;
$count -= 3;
$count *= 2;
$count /= 4;

// ── Pre and post increment ────────────────────────────────────
++$count;
$count++;
--$count;
$count--;

// ── Arrays ───────────────────────────────────────────────────
$array[0] = 100;
$array[1] = 200;
$val = $array[0];
$idx = 1;
$val = $array[$idx];
$array[$count] = $result;

// ── Tables ───────────────────────────────────────────────────
$table["key"]  = "value";
$table[1]      = TRUE;
$table[$count] = $result;
$tval = $table["key"];

// ── Multi-dimensional arrays ──────────────────────────────────
$grid[0][0] = 1;
$grid[1][2] = $count;
$gval = $grid[0][0];

// ── Arrays in conditions ──────────────────────────────────────
if ($array[0])
{
    $count = $array[0];
}

if ($table["key"])
{
    $result = 1;
}

// ── Array utility functions ───────────────────────────────────
$size = arraySize($array);
$keys = tableKeys($table);

// ── While loop — simple ───────────────────────────────────────
$i = 0;
while ($i < 10)
{
    $i += 1;
}

// ── While with post-increment in condition ────────────────────
$i = 0;
while ($i++ < 5)
{
    $result = $i;
}

// ── While with pre-increment in condition ────────────────────
$i = 0;
while (++$i < 5)
{
    $result = $i;
}

// ── While with function in condition ─────────────────────────
$i = 0;
while (arraySize($array) < 10)
{
    $array[$i] = $i;
    $i += 1;
}

// ── Continue inside while ────────────────────────────────────
$i = 0;
while ($i < 10)
{
    $i += 1;
    if ($i == 5)
    {
        continue;
    }
    $result = $i;
}

// ── Continue in single-line if inside while ──────────────────
$i = 0;
while ($i++ < 10)
{
    if ($i == 3) continue;
    $result = $i;
}

// ── Break inside while ───────────────────────────────────────
$i = 0;
while ($i < 100)
{
    if ($i >= 10)
    {
        break;
    }
    $i += 1;
}

// ── Single-line while (no braces) ────────────────────────────
$i = 0;
while ($i < 5) $i += 1;

// ── Nested while loops ───────────────────────────────────────
$i = 0;
while ($i < 3)
{
    $j = 0;
    while ($j < 3)
    {
        $grid[$i][$j] = $i + $j;
        $j += 1;
    }
    $i += 1;
}

// ── Statements before while block ────────────────────────────
wait(30);
while ($i < 10)
{
    if ($i == 5) continue;
    wait(10);
    $i += 1;
}

// ── Labels, goto, gosub ──────────────────────────────────────
gosub initialise;

$result = $myVar;
$flag = TRUE;
goto cleanup;

initialise:
    $myVar = 0;
    $myVar += 1;
endsub;

cleanup:
    $count = 0;

// ── Array with post-increment in subscript ───────────────────
$i = 0;
$array[$i++] = 10;
$array[$i++] = 20;

// ── Mixed inc/dec in single expression ───────────────────────
$i = 0;
$j = 5;
$array[$i++] = --$j;
$array[$i]   = $j++;

// ── Defines ──────────────────────────────────────────────────

#define MAX_COUNT 100
#define ADD(a, b) a + b
#define CLAMP(v, lo, hi) (v < lo) ? lo : (v > hi) ? hi : v

//$limit = MAX_COUNT;
$sum   = ADD($count, 10);

// ── Not modifier ─────────────────────────────────────────────
if not ($count > MAX_COUNT)
{
    $count += 1;
}

// ── Return values ─────────────────────────────────────────────
return $count;
