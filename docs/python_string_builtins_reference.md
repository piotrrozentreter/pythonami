# Python String and Character Built-ins Reference

**Scope:** Python 3 public `str` methods, import-free built-in functions relevant to text, and language-level string operations.  
**For:** implementation planning and testing of a custom interpreter such as PythonAmi.

> Python has no separate `char` type. A character is a `str` of length one. Strings are immutable, so methods return new values.

## 1. Import-free built-in functions relevant to strings

### Core conversion and representation

- `str(object='')`: Convert an object to text. The full form `str(bytes, encoding, errors)` decodes bytes.
- `repr(object)`: Return an interpreter-oriented representation, normally with quotes and escapes.
- `ascii(object)`: Like `repr()`, but escape non-ASCII characters.
- `format(value, format_spec='')`: Format a value according to a format specification.
- `bool(s)`: `False` for `''`; `True` for every non-empty string.

```python
str(123)                 # '123'
repr('A\nB')             # "'A\\nB'"
ascii('cafe\u0301')      # escaped non-ASCII representation
format(42, '04d')        # '0042'
bool('')                 # False
bool('0')                # True
```

### Character and Unicode conversion

- `ord(character)`: Return the Unicode code point of a one-character string.
- `chr(code_point)`: Return the character for an integer code point from `0` to `0x10FFFF`.
- `bytes(text, encoding, errors='strict')`: Encode text to immutable bytes.
- `bytearray(text, encoding, errors='strict')`: Encode text to mutable bytes.

```python
ord('A')                 # 65
chr(65)                  # 'A'
ord('\u0141')            # 321
chr(0x0141)              # '\u0141'
bytes('ABC', 'ascii')     # b'ABC'
```

### Sequence and iteration helpers

- `len(s)`: Number of Unicode code points.
- `list(s)`: List of one-character strings.
- `tuple(s)`: Tuple of one-character strings.
- `iter(s)`: Character iterator.
- `next(iterator, default)`: Next character or optional default.
- `enumerate(s, start=0)`: `(index, character)` pairs.
- `reversed(s)`: Reverse character iterator.
- `sorted(s, key=None, reverse=False)`: New sorted list of characters.

```python
len('ABC')                       # 3
list('ABC')                      # ['A', 'B', 'C']
tuple('AB')                      # ('A', 'B')
list(enumerate('AB'))            # [(0, 'A'), (1, 'B')]
''.join(reversed('ABC'))         # 'CBA'
''.join(sorted('cab'))           # 'abc'
```

### Aggregation and predicates

- `min(s)`: Lowest character by Unicode code point; empty input raises `ValueError`.
- `max(s)`: Highest character by Unicode code point; empty input raises `ValueError`.
- `all(iterable)`: `True` if all elements are truthy; also `True` for an empty iterable.
- `any(iterable)`: `True` if any element is truthy; `False` for an empty iterable.

### Text input and output

- `input(prompt='')`: Read a line and return it without the trailing newline; end of input raises `EOFError`.
- `print(*objects, sep=' ', end='\n', file=None, flush=False)`: Convert objects to text and write them.

### Dynamic source processing

- `eval(source, globals=None, locals=None)`: Evaluate an expression supplied as text.
- `exec(source, globals=None, locals=None)`: Execute statements supplied as text.
- `compile(source, filename, mode, flags=0, dont_inherit=False, optimize=-1)`: Compile source text.

These three are security-sensitive and can be deferred in a new interpreter.

---

## 2. All public `str` methods

### Case conversion

#### `str.capitalize()`
Uppercase the first character and lowercase the remainder.

```python
'hello WORLD'.capitalize()       # 'Hello world'
```

#### `str.casefold()`
Aggressive Unicode case normalization for caseless matching.

```python
'Stra\u00dfe'.casefold()         # 'strasse'
```

#### `str.lower()`
Return a lowercase copy.

#### `str.swapcase()`
Swap lowercase and uppercase characters.

#### `str.title()`
Return a title-cased copy.

#### `str.upper()`
Return an uppercase copy. Unicode conversion can change length, for example `'\u00df'.upper()` is `'SS'`.

### Alignment and padding

#### `str.center(width, fillchar=' ')`
Center in a field of at least `width` characters.

#### `str.ljust(width, fillchar=' ')`
Left-align in a field.

#### `str.rjust(width, fillchar=' ')`
Right-align in a field.

#### `str.zfill(width)`
Pad a numeric string with zeroes after an optional sign.

```python
'cat'.center(7)                  # '  cat  '
'cat'.ljust(6, '.')              # 'cat...'
'cat'.rjust(6, '.')              # '...cat'
'-42'.zfill(5)                   # '-0042'
```

`fillchar` must contain exactly one character.

### Searching and counting

#### `str.count(sub[, start[, end]])`
Count non-overlapping occurrences in the selected range.

#### `str.find(sub[, start[, end]])`
Return the lowest matching index, or `-1`.

#### `str.rfind(sub[, start[, end]])`
Return the highest matching index, or `-1`.

#### `str.index(sub[, start[, end]])`
Like `find()`, but raise `ValueError` when absent.

#### `str.rindex(sub[, start[, end]])`
Like `rfind()`, but raise `ValueError` when absent.

#### `str.startswith(prefix[, start[, end]])`
Test a string or tuple of prefixes.

#### `str.endswith(suffix[, start[, end]])`
Test a string or tuple of suffixes.

```python
'banana'.count('an')             # 2
'banana'.find('na')              # 2
'banana'.rfind('na')             # 4
'python.py'.startswith(('js', 'py'))  # True
'python.py'.endswith(('.py', '.pyw')) # True
```

### Trimming and affix removal

#### `str.strip(chars=None)`
Remove leading and trailing whitespace, or members of the supplied character set.

#### `str.lstrip(chars=None)`
Remove from the left.

#### `str.rstrip(chars=None)`
Remove from the right.

#### `str.removeprefix(prefix)`
Remove exactly one matching prefix.

#### `str.removesuffix(suffix)`
Remove exactly one matching suffix.

```python
'  hello  '.strip()              # 'hello'
'xyhelloxy'.strip('xy')          # 'hello'
'TestCase'.removeprefix('Test')  # 'Case'
'image.png'.removesuffix('.png') # 'image'
```

`strip('abc')` treats its argument as a set of characters, not one literal affix.

### Splitting, partitioning, and joining

#### `str.split(sep=None, maxsplit=-1)`
Split from the left. With `sep=None`, combine whitespace runs and ignore edge whitespace.

#### `str.rsplit(sep=None, maxsplit=-1)`
Split from the right.

#### `str.splitlines(keepends=False)`
Split at Unicode line boundaries.

#### `str.partition(sep)`
Return `(before, separator, after)` using the first match.

#### `str.rpartition(sep)`
Return the same triple using the last match.

#### `str.join(iterable)`
Join string elements using the receiver as separator.

```python
'  a   b  '.split()              # ['a', 'b']
'a,b,c'.split(',', 1)            # ['a', 'b,c']
'a,b,c'.rsplit(',', 1)           # ['a,b', 'c']
'a\nb'.splitlines(True)          # ['a\n', 'b']
'a=b=c'.partition('=')           # ('a', '=', 'b=c')
'a=b=c'.rpartition('=')          # ('a=b', '=', 'c')
'-'.join(['a', 'b', 'c'])        # 'a-b-c'
```

An empty explicit separator raises `ValueError`. `join()` raises `TypeError` if an item is not a string.

### Replacement and translation

#### `str.replace(old, new, count=-1)`
Replace occurrences, optionally limiting their count.

#### `str.maketrans(x[, y[, z]])`
Static method that creates a translation table from a mapping or paired strings. Optional `z` lists characters to delete.

#### `str.translate(table)`
Translate code points using integer keys. Values may be an integer code point, a replacement string, or `None` for deletion.

```python
'banana'.replace('a', 'A', 2)    # 'bAnAna'
table = str.maketrans('abc', 'ABC', '!')
'a!bc'.translate(table)          # 'ABC'
```

### Tabs and encoding

#### `str.expandtabs(tabsize=8)`
Replace tabs with spaces according to tab stops.

#### `str.encode(encoding='utf-8', errors='strict')`
Encode to `bytes`.

```python
'A\tB'.expandtabs(4)             # 'A   B'
'ABC'.encode('ascii')            # b'ABC'
'\u0141'.encode('utf-8')         # b'\xc5\x81'
```

Encoding may raise `UnicodeEncodeError`; unknown encodings or handlers may raise `LookupError`.

### Formatting

#### `str.format(*args, **kwargs)`
Apply replacement-field formatting.

#### `str.format_map(mapping)`
Format using a mapping directly.

```python
'Hello, {}!'.format('Piotr')
'{name}: {value:04d}'.format(name='x', value=7)  # 'x: 0007'
'{name}={value}'.format_map({'name': 'x', 'value': 7})
```

F-strings are syntax, not a `str` method.

### Character classification

#### `str.isalnum()`
`True` when non-empty and all characters are alphabetic, decimal, digit, or numeric.

#### `str.isalpha()`
`True` when non-empty and all characters are alphabetic.

#### `str.isascii()`
`True` when empty or every code point is from U+0000 through U+007F.

#### `str.isdecimal()`
`True` when non-empty and every character is a Unicode decimal character.

#### `str.isdigit()`
`True` when non-empty and every character is a Unicode digit.

#### `str.isidentifier()`
`True` when lexically valid as a Python identifier. It does not reject reserved keywords.

#### `str.islower()`
`True` when all cased characters are lowercase and at least one cased character exists.

#### `str.isnumeric()`
`True` when non-empty and every character is Unicode numeric.

#### `str.isprintable()`
`True` when empty or every character is printable.

#### `str.isspace()`
`True` when non-empty and every character is whitespace.

#### `str.istitle()`
`True` when the cased characters follow title-case rules and at least one cased character exists.

#### `str.isupper()`
`True` when all cased characters are uppercase and at least one cased character exists.

```python
'abc123'.isalnum()               # True
'Za\u017c\u00f3\u0142\u0107'.isalpha() # True
''.isascii()                     # True
'123'.isdecimal()                # True
'\u00b2'.isdigit()              # True
'variable_1'.isidentifier()      # True
'class'.isidentifier()           # True
'abc123'.islower()               # True
'\u2155'.isnumeric()            # True
''.isprintable()                 # True
' \t\n'.isspace()              # True
'Hello World'.istitle()          # True
'ABC123'.isupper()               # True
```

Numeric classification relationship:

```text
isdecimal subset of isdigit subset of isnumeric
```

Most classification methods return `False` for `''`; notable exceptions are `isascii()` and `isprintable()`, which return `True`.

---

## 3. String operators and syntax

### Concatenation and repetition

```python
'Py' + 'thon'                    # 'Python'
'ab' * 3                        # 'ababab'
3 * 'ab'                        # 'ababab'
'ab' * 0                        # ''
'ab' * -2                       # ''
```

### Indexing and slicing

```python
s = 'Python'
s[0]                             # 'P'
s[-1]                            # 'n'
s[1:4]                           # 'yth'
s[:3]                            # 'Pyt'
s[3:]                            # 'hon'
s[::-1]                          # 'nohtyP'
s[::2]                           # 'Pto'
```

Invalid indexing raises `IndexError`; a zero slice step raises `ValueError`.

### Containment

```python
'th' in 'Python'                 # True
'x' not in 'Python'              # True
'' in 'Python'                   # True
```

### Comparisons

Supported operators are `==`, `!=`, `<`, `<=`, `>`, and `>=`. Ordering is lexicographic by Unicode code point, not locale-aware collation.

```python
'abc' < 'abd'                    # True
'A' < 'a'                        # True
```

### Iteration

```python
for ch in 'ABC':
    print(ch)
```

Each `ch` is a one-character string.

### Immutability

```python
s = 'cat'
s[0] = 'b'                      # TypeError
s = 'b' + s[1:]                 # 'bat'
```

---

## 4. String literals and escapes

### Literal forms

```python
'hello'
"hello"
'''multiline
text'''
r'C:\new\test'
f'{2 + 3 * 4}'                  # '14'
```

Common prefixes:

- `r` or `R`: raw string
- `f` or `F`: formatted string
- `u` or `U`: Unicode compatibility prefix
- `b` or `B`: bytes literal, not `str`
- `fr` and `rf`: formatted raw string combinations

### Escape sequences

```text
\\          backslash
\'          single quote
\"          double quote
\a          bell
\b          backspace
\f          form feed
\n          line feed
\r          carriage return
\t          horizontal tab
\v          vertical tab
\ooo        octal value
\xhh        two-digit hexadecimal value
\N{name}    named Unicode character
\uxxxx      four-digit Unicode code point
\Uxxxxxxxx  eight-digit Unicode code point
```

Adjacent literals concatenate at compile time:

```python
s = 'Hello, ' 'world!'          # 'Hello, world!'
```

---

## 5. Important errors and edge cases

```python
len('')                          # 0
''.isalpha()                     # False
''.isascii()                     # True
''.isprintable()                 # True
''.split()                       # []
''.split(',')                    # ['']
''.join([])                      # ''
'abc'.count('')                  # 4
'abc'.find('x')                  # -1
'abc'.index('x')                 # ValueError
'abc'[3]                         # IndexError
'abc'[1.0]                       # TypeError
'abc'[::0]                       # ValueError
'abc'.split('')                  # ValueError
'abc'.partition('')              # ValueError
'a' + 1                          # TypeError
'-'.join(['a', 2])               # TypeError
ord('AB')                        # TypeError
chr(-1)                          # ValueError
chr(0x110000)                    # ValueError
'\u0141'.encode('ascii')         # UnicodeEncodeError
```

A displayed character can contain multiple Unicode code points:

```python
len('\u00e9')                    # 1, precomposed
len('e\u0301')                  # 2, e plus combining accent
```

---

## 6. Recommended implementation order for PythonAmi

### Phase 1: minimum viable strings

- String literals and escapes
- `str`, `print`, `input`, `len`, `ord`, `chr`
- `+`, `*`, truth testing, comparisons
- Indexing, negative indexing, slicing
- `in`, `not in`, and iteration

### Phase 2: high-value methods

- `upper`, `lower`, `capitalize`, `swapcase`
- `find`, `rfind`, `index`, `rindex`, `count`
- `startswith`, `endswith`
- `strip`, `lstrip`, `rstrip`
- `split`, `rsplit`, `join`, `splitlines`
- `replace`
- Common `is...` classification methods

### Phase 3: compatibility

- `partition`, `rpartition`
- `removeprefix`, `removesuffix`
- `center`, `ljust`, `rjust`, `zfill`
- `title`, `casefold`, remaining classifiers
- `expandtabs`, `maketrans`, `translate`

### Phase 4: advanced behavior

- `format`, `format_map`, f-strings
- `encode`, byte conversion
- Full Unicode mappings and classifications
- Exact `repr` and `ascii` escaping
- `eval`, `exec`, and `compile`, if needed

---

## 7. Compact implementation checklist

### All public `str` methods

```text
[x] capitalize    [x] casefold       [x] center
[x] count         [ ] encode         [x] endswith
[x] expandtabs    [x] find           [ ] format
[ ] format_map    [x] index          [x] isalnum
[x] isalpha       [x] isascii        [x] isdecimal
[x] isdigit       [x] isidentifier   [x] islower
[x] isnumeric     [x] isprintable    [x] isspace
[x] istitle       [x] isupper        [x] join
[x] ljust         [x] lower          [x] lstrip
[x] maketrans     [x] partition      [x] removeprefix
[x] removesuffix  [x] replace        [x] rfind
[x] rindex        [x] rjust          [x] rpartition
[x] rsplit        [x] rstrip         [x] split
[x] splitlines    [x] startswith     [x] strip
[x] swapcase      [x] title          [x] translate
[x] upper         [x] zfill
```

Notes: `encode` deferred (no `bytes`). `format` / `format_map` deferred as methods; minimal builtin `format(value, spec)` covers int padding. `maketrans` is a builtin (no `str` type object). `casefold` is ASCII `lower`. Classifiers are ASCII/8-bit only (see D-0024).

### Import-free built-ins relevant to text

```text
[x] all       [x] any       [x] ascii      [x] bool
[ ] bytearray [ ] bytes     [x] chr        [ ] compile
[ ] enumerate [ ] eval      [ ] exec       [x] format
[x] input     [ ] iter      [x] len        [x] list
[x] max       [x] min       [ ] next       [x] ord
[x] print     [x] repr      [ ] reversed   [ ] sorted
[x] str       [x] tuple
```

Notes: `bytes`/`bytearray`/`eval`/`exec`/`compile` deferred. Iterator helpers deferred; `for` already iterates strings. `format` is minimal int specs only. `maketrans` is also installed as a builtin (see above).

### Operators and language features

```text
[x] + concatenation             [x] * repetition
[x] == != < <= > >= comparisons [x] [] indexing
[x] [start:stop] slicing (no step)  [x] in / not in
[x] string iteration            [x] truth value
[ ] adjacent literals           [x] escape sequences
[ ] raw strings                 [ ] f-strings
[x] immutability errors
```

Notes: Escapes decoded: `\\ \' \" \n \r \t \xHH`. Adjacent literals, raw strings, and f-strings remain deferred.
Implemented via `OP_CONTAINS` / `OP_NOT_CONTAINS` and string conversion in `OP_RANGE_INIT` (D-0036).
---

## Scope and official references

This reference includes public `str` methods, string syntax, and import-free built-ins that directly construct, inspect, iterate, encode, display, or process text. It excludes import-only modules such as `string`, `re`, `textwrap`, `unicodedata`, `codecs`, and `keyword`, as well as private and most dunder methods.

- [Python Text Sequence Type: str](https://docs.python.org/3/library/stdtypes.html#text-sequence-type-str)
- [Python Built-in Functions](https://docs.python.org/3/library/functions.html)
- [Python Built-in Types](https://docs.python.org/3/library/stdtypes.html)
- [Python String and Bytes Literals](https://docs.python.org/3/reference/lexical_analysis.html#string-and-bytes-literals)
- [Python Format String Syntax](https://docs.python.org/3/library/string.html#format-string-syntax)
