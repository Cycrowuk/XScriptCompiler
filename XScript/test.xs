// ============================================================
//  XScript Compiler v0.8 Feature Test
//  Tests all language features including v0.8 additions:
//  function main() wrapper, sub blocks, local-scoped functions
// ============================================================

#DESCRIPTION "XScript v0.8 Feature Test Script"
#VERSION 1
#COMMAND 0

// ── Basic #define ─────────────────────────────────────────────
#define MAX_COUNT 100
#define ADD(a, b) a + b
#define CLAMP(v, lo, hi) (v < lo) ? lo : (v > hi) ? hi : v

// ── Presence-only define for #ifdef ───────────────────────────
#define FEATURE_ENABLED

// ── #ifdef / #ifndef / #elseif / #else / #endif ───────────────
#ifdef FEATURE_ENABLED
    #define PLATFORM_VALUE 1
#elseif MISSING_SYMBOL
    #define PLATFORM_VALUE 2
#else
    #define PLATFORM_VALUE 3
#endif

// ── User-defined functions (v0.8) ─────────────────────────────

// Function with explicit pardef types
function DATATYPE_INT hasSector(VARSECTOR $sector)
{
    if ($sector == NULL)
    {
        return(FALSE);
    }
    return(TRUE);
}

// Function with no pardef (defaults to VALUE)
function getDoubled($value)
{
    return($value * 2);
}

// Function with multiple parameters, mixed typed and untyped
function DATATYPE_INT clampValue($value, DATATYPE_INT $lo, DATATYPE_INT $hi)
{
    if ($value < $lo)
    {
        return($lo);
    }
    if ($value > $hi)
    {
        return($hi);
    }
    return($value);
}

// void function (no return value used)
function DATATYPE_NULL logValue($value)
{
    $fn.logValue.output = $value;
}

// ── Subs (v0.8 block syntax) ──────────────────────────────────

// Sub before main — should be deferred and emitted after main
sub initCounters()
{
    $initCount = 0;
    $initFlag = TRUE;
}

// ── main function (required, v0.8) ────────────────────────────
function main(VARSECTOR $sector, SHIP $ship)
{
    // ── #ifdef inside function body ───────────────────────────
    #ifdef FEATURE_ENABLED
    $debugMode = TRUE;
    #else
    $debugMode = FALSE;
    #endif

    #ifndef MISSING_SYMBOL
    $flag = TRUE;
    #endif

    // ── Basic variables and assignment ────────────────────────
    $count = 0;
    $name = "test ship";
    $empty = NULL;

    // ── Constants ─────────────────────────────────────────────
    $player = PLAYERSHIP;
    $owner  = ThisOwner;
    $raceFlag = Xenon;
    $page = TextPage::MiscVoice;

    // ── User-defined function calls (v0.8) ────────────────────

    // Bare call (no return value)
    logValue($count);

    // Call with return value
    $doubled = getDoubled($count);

    // Call used in expression context (assignment)
    $clamped = clampValue($count, 0, MAX_COUNT);

    // Call with sector argument
    $sectorExists = hasSector($sector);

    // Call inside if condition block
    if ($sectorExists)
    {
        $count += 1;
    }

    // ── Gosub for plain sub (v0.8 block syntax, called before main) ──
    gosub initCounters;
    $count += $initCount;

    // ── Simple if/else if/else ────────────────────────────────
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

    // ── Nested function calls as arguments ────────────────────
    $result = random($count + 10);

    // ── if with function in condition ─────────────────────────
    if (random($count + 5) > 3)
    {
        $count += 1;
    }

    // ── Compound assignment ───────────────────────────────────
    $count += 10;
    $count -= 3;
    $count *= 2;
    $count /= 4;

    // ── Pre and post increment ────────────────────────────────
    ++$count;
    $count++;
    --$count;
    $count--;

    // ── Arrays ───────────────────────────────────────────────
    $array[0] = 100;
    $array[1] = 200;
    $val = $array[0];
    $idx = 1;
    $val = $array[$idx];
    $array[$count] = $result;

    // ── Tables ───────────────────────────────────────────────
    $table["key"]  = "value";
    $table[1]      = TRUE;
    $table[$count] = $result;
    $tval = $table["key"];

    // ── Multi-dimensional arrays ──────────────────────────────
    $grid[0][0] = 1;
    $grid[1][2] = $count;
    $gval = $grid[0][0];

    // ── While loop ───────────────────────────────────────────
    $i = 0;
    while ($i < 10)
    {
        $i += 1;
    }

    // ── While with post-increment in condition ────────────────
    $i = 0;
    while ($i++ < 5)
    {
        $result = $i;
    }

    // ── Continue inside while ────────────────────────────────
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

    // ── Break inside while ───────────────────────────────────
    $i = 0;
    while ($i < 100)
    {
        if ($i >= 10)
        {
            break;
        }
        $i += 1;
    }

    // ── Single-line while (no braces) ────────────────────────
    $i = 0;
    while ($i < 5) $i += 1;

    // ── Nested while loops ───────────────────────────────────
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

    // ── Plain label/goto/gosub (existing syntax) ─────────────
    gosub doWork;

    $result = $workResult;
    $flag = TRUE;
    goto cleanup;

    cleanup:
        $count = 0;

    // ── Array utility functions ───────────────────────────────
    $size = arraySize($array);
    $keys = tableKeys($table);

    // ── Array with post-increment in subscript ───────────────
    $i = 0;
    $array[$i++] = 10;
    $array[$i++] = 20;

    // ── #define usage ─────────────────────────────────────────
    $limit = MAX_COUNT;
    $sum   = ADD($count, 10);

    // ── Not modifier ─────────────────────────────────────────
    if not ($count > MAX_COUNT)
    {
        $count += 1;
    }

    // ── Conditional compilation with #define ─────────────────
    #ifdef FEATURE_ENABLED
    $count += PLATFORM_VALUE;
    #endif

    return($count);
}

// ── Sub after main (v0.8 block syntax) ───────────────────────
sub doWork()
{
    $workResult = 42;
    $workResult += 1;
    // early exit via endsub
    if ($workResult > 100)
    {
        endsub;
    }
    $workResult *= 2;
}

// ── Second user function defined after main ───────────────────
// (tests that forward declarations work for calls made before this point)
function DATATYPE_INT tripleValue($value)
{
    $tripled = $value * 3;
    return($tripled);
}
