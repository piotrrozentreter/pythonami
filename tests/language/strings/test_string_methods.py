# String methods and text builtins (8-bit ASCII Language Level).

print("=== case ===")
print("hello WORLD".capitalize())
print("AbC".upper())
print("AbC".lower())
print("AbC".swapcase())
print("hello world".title())
print("AbC".casefold())

print("=== search ===")
print("banana".count("an"))
print("banana".find("na"))
print("banana".rfind("na"))
print("banana".index("na"))
print("python.py".startswith(("js", "py")))
print("python.py".endswith((".py", ".pyw")))
print("abc".count(""))
print("abc".find("x"))

print("=== trim ===")
print("  hello  ".strip())
print("xyhelloxy".strip("xy"))
print("TestCase".removeprefix("Test"))
print("image.png".removesuffix(".png"))

print("=== split join ===")
print("  a   b  ".split())
print("a,b,c".split(",", 1))
print("a,b,c".rsplit(",", 1))
lines = "a\nb".splitlines(True)
print(len(lines))
print(len(lines[0]))
print(len(lines[1]))
print("a\nb".splitlines())
parts = "a=b=c".partition("=")
print(parts[0])
print(parts[1])
print(parts[2])
rparts = "a=b=c".rpartition("=")
print(rparts[0])
print(rparts[1])
print(rparts[2])
print("-".join(["a", "b", "c"]))

print("=== replace align tabs ===")
print("banana".replace("a", "A", 2))
print("cat".center(7))
print("cat".ljust(6, "."))
print("cat".rjust(6, "."))
print("-42".zfill(5))
print("A\tB".expandtabs(4))
print(len("A\tB".expandtabs(4)))

print("=== translate ===")
table = maketrans("abc", "ABC", "!")
print("a!bc".translate(table))

print("=== classifiers ===")
print("abc123".isalnum())
print("abc".isalpha())
print("".isascii())
print("123".isdecimal())
print("123".isdigit())
print("variable_1".isidentifier())
print("class".isidentifier())
print("abc123".islower())
print("123".isnumeric())
print("".isprintable())
print(" \t\n".isspace())
print("Hello World".istitle())
print("ABC123".isupper())
print("".isalpha())

print("=== builtins ===")
print(ord("A"))
print(chr(65))
print(repr("A\nB"))
print(ascii("A"))
print(format(42, "04d"))
print(format(7, "d"))
print(all([1, 2, 3]))
print(all([1, 0, 3]))
print(any([0, 0, 1]))
print(any([]))
print(all([]))

print("=== errors ===")
caught = 0
try:
    "abc".index("x")
except ValueError:
    caught = 1
print(caught)
caught = 0
try:
    "abc".split("")
except ValueError:
    caught = 1
print(caught)
caught = 0
try:
    "abc".partition("")
except ValueError:
    caught = 1
print(caught)
caught = 0
try:
    ord("AB")
except TypeError:
    caught = 1
print(caught)
caught = 0
try:
    chr(256)
except ValueError:
    caught = 1
print(caught)
caught = 0
try:
    "-".join(["a", 2])
except TypeError:
    caught = 1
print(caught)
