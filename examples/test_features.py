# Python68K Incremental Feature Test Suite
# Target: AmigaOS 2.x+ (Motorola 68000)

print("=== 1. Scalar Arithmetic ===")
a = 10
b = 3
print(a + b)
print(a - b)
print(a * b)
print(a // b)
print(a % b)

print("=== 2. Comparisons & Logic ===")
print(a > b)
print(a == 10)
print(b != 3)

print("=== 3. Conditional Statements ===")
if a > 5:
    print("a is greater than 5")
else:
    print("a is not greater than 5")

print("=== 4. While Loops ===")
counter = 3
while counter > 0:
    print(counter)
    counter = counter - 1

print("=== 5. Builtin Functions & Lists ===")
items = [10, 20, 30]
print(items)
print(len(items))
print(items[1])
popped = list_pop(items)
print(popped)
print(items)
print(len(items))

print("=== 6. For Loops & Range ===")
total = 0
for i in range(5):
    total = total + i
print(total)

r_total = 0
for x in range(1, 10, 2):
    r_total = r_total + x
print(r_total)

print("=== 7. Augmented Assignment ===")
c = 10
c += 5
print(c)
c -= 3
print(c)
c *= 2
print(c)
c //= 4
print(c)
c %= 4
print(c)

print("=== 8. Function Definitions ===")


def add(a, b):
    return a + b


print(add(3, 4))


def fibonacci(n):
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


fib_index = 0
while fib_index < 10:
    print(fibonacci(fib_index))
    fib_index = fib_index + 1


def sum_up_to(n):
    total = 0
    for i in range(n):
        total += i
    return total


print(sum_up_to(6))

print("=== 9. Lists with list_append ===")
grown = [1, 2]
list_append(grown, 3)
list_append(grown, 4)
print(grown)
print(len(grown))

print("=== 10. Short-circuit and/or ===")
print(1 and 2)
print(0 or 5)
print(0 and 9)
print(4 or 0)

print("=== 11. Strings ===")
print("ab" + "cd")
text = "hello"
print(text[1])
print(text[1:4])
print(len(text))

print("=== 12. Remaining Builtins ===")
print(int("42"))
print(str(7))
print(bool(""))
print(bool(1))
print(abs(-3))
print(min(2, 9))
print(max(2, 9))

print("=== 13. Floor/Mod Negatives ===")
print((-7) // 3)
print((-7) % 3)
print(7 // (-3))
print(7 % (-3))

print("=== 14. Truthiness & not ===")
print([] and 1)
print([1] and 2)
print("" or "x")
print(None or 0 or 5)
print(not [])
print(not [1])
print(not "")

print("=== 15. Elif & while-else ===")
x = 2
if x == 1:
    print("one")
elif x == 2:
    print("two")
else:
    print("other")
i = 0
while i < 2:
    print(i)
    i = i + 1
else:
    print("while-done")

print("=== 16. Nested Loops & continue ===")
nested = 0
for a in range(3):
    for b in range(3):
        if b == 1:
            continue
        nested = nested + a * 10 + b
print(nested)

print("=== 17. Recursion & list build ===")


def squares(n):
    out = []
    for i in range(n):
        list_append(out, i * i)
    return out


def fact(n):
    if n <= 1:
        return 1
    return n * fact(n - 1)


print(squares(4))
print(fact(5))

print("=== 18. Range reverse ===")
rev = 0
for n in range(6, 0, -1):
    rev = rev + n
print(rev)

print("=== 19. for-break ===")
broken = 0
for n in range(8):
    if n == 5:
        break
    broken = broken + n
print(broken)

print("=== 20. Index Assignment ===")
vals = [1, 2, 3]
vals[0] = 9
vals[2] = 7
print(vals[0])
print(vals[1])
print(vals[2])
grid = [[0, 0], [0, 0]]
grid[0][1] = 5
grid[1][0] = 6
print(grid[0][1])
print(grid[1][0])

print("=== 21. Bool/None Equality ===")
print(None == None)
print(None == 1)
print(True == 1)
print(False == 0)
print(True != False)

print("=== 22. File Read/Write ===")
feat_path = "py68k_feat_io.tmp"
f = fopen(feat_path, "w")
fwrite(f, "Python68K")
fclose(f)
print(exists(feat_path))
f = fopen(feat_path, "r")
print(fread(f, 9))
fclose(f)
f = fopen(feat_path, "a")
fwrite(f, "-IO")
fclose(f)
f = fopen(feat_path, "rb")
print(fread(f, 20))
fclose(f)
feat_renamed = "py68k_feat_io_renamed.tmp"
rename(feat_path, feat_renamed)
print(exists(feat_path))
print(exists(feat_renamed))
remove(feat_renamed)
print(exists(feat_renamed))

print("=== 23. Amiga Assigns ===")
assign_add("PY68KFEAT", "RAM:")
print(assign_get("PY68KFEAT") != None)
assign_remove("PY68KFEAT")
print(assign_get("PY68KFEAT") == None)

print("=== 24. Command Arguments ===")
import sys
print(sys.argv)
print(sys.argv[0])

print("=== 25. String Methods & Text Builtins ===")
print("AbC".upper())
print("banana".find("na"))
print("  hi  ".strip())
print("-".join(["a", "b"]))
print("a,b".split(","))
print(ord("A"))
print(chr(65))
print(format(42, "04d"))
print(all([1, 2]))
print(any([0, 1]))
print("hello".startswith("he"))
print("123".isdigit())
table = maketrans("ab", "AB")
print("ab".translate(table))

print("=== 26. Comprehensions ===")
print([x * x for x in range(4)])
print([x for x in range(5) if x % 2 == 1])
print({x for x in range(3)} == {0, 1, 2})
squares = {n: n * n for n in range(3)}
print(squares[2])
print([a + b for a in range(2) for b in range(2)])

print("=== 27. Identity is / is not ===")
print(None is None)
print(1 is None)
print(True is True)
print(True is 1)
left = [1, 2]
right = [1, 2]
print(left == right)
print(left is right)
print(left is not right)
print(0 is not 1)
print(0 is (not 1))

print("=== 28. Unpacking / multiple assignment ===")
u, v = (1, 2)
print(u)
print(v)
u, v = [3, 4]
print(u + v)
u, v = 5, 6
print(u)
print(v)
swap_l = 1
swap_r = 2
swap_l, swap_r = swap_r, swap_l
print(swap_l)
print(swap_r)
pair_sum = 0
for x, y in [(10, 1), (20, 2)]:
    pair_sum = pair_sum + x + y
print(pair_sum)
unpack_err = 0
try:
    too_few, extra = [1]
except ValueError:
    unpack_err = 1
print(unpack_err)

print("=== 29. Conditional expressions ===")
print("yes" if 1 else "no")
print("yes" if 0 else "no")
print("a" if 0 else "b" if 0 else "c")
print(1 if 1 or 0 else 2)
print([x if x % 2 else -x for x in range(4)])
print(len("ab" if 1 else "abcd"))

print("=== literals ===")
print(f"hi")
print(f"")
print(f"{{ok}}")
print(f"a{{b}}c")

print("=== expressions ===")
x = 42
print(f"x={x}")
print(f"a{1 + 2}b")
name = "Ada"
print(f"Hello, {name}!")

print("=== conversions ===")
s = "A\nB"
print(f"{s!r}")
print(f"{s!a}")
print(f"{x!s}")

print("=== format specs ===")
print(f"{x:04d}")
print(f"{7:d}")

# iter / next / StopIteration

print("=== list iter ===")
it = iter([1, 2, 3])
print(next(it))
print(next(it))
print(next(it))
caught = 0
try:
    next(it)
except StopIteration:
    caught = 1
print(caught)
print(next(it, 99))

print("=== tuple and string ===")
it2 = iter((7, 8))
print(next(it2))
print(next(it2, -1))
print(next(it2, -1))
chars = iter("ab")
print(next(chars))
print(next(chars))

print("=== for over iter ===")
total = 0
for x in iter([10, 20, 30]):
    total = total + x
print(total)

print("=== double iter identity ===")
base = iter([1])
same = iter(base)
print(same is base)
print(next(same))

print("=== errors ===")
bad = 0
try:
    iter(5)
except TypeError:
    bad = 1
print(bad)
bad = 0
try:
    next(5)
except TypeError:
    bad = 1
print(bad)

print("=== Feature Test Complete ===")
