# Language Level 0.7 generator expressions.
#
# Every section here is differential against CPython except the one marked
# "freevar snapshot", which documents the D-0046 divergence: Python68K has no
# cells, so a generator expression captures enclosing locals by value when it
# is created.

print("=== basic ===")
g = (x for x in range(4))
for v in g:
    print(v)

print("=== expression element ===")
for v in (x * x for x in range(1, 5)):
    print(v)

print("=== filter ===")
for v in (x for x in range(10) if x % 3 == 0):
    print(v)

print("=== multiple filters ===")
for v in (x for x in range(20) if x % 2 == 0 if x % 3 == 0):
    print(v)

print("=== nested for clauses ===")
for v in (a * 10 + b for a in range(3) for b in range(2)):
    print(v)

print("=== inner iterable depends on the outer name ===")
for v in (a * 10 + b for a in range(3) for b in range(a)):
    print(v)

print("=== laziness ===")
lazy = (x for x in range(3))
print("created")
print(next(lazy))
print("stepped once")
print(next(lazy))
print(next(lazy))
print(next(lazy, -1))

print("=== generator expression is its own iterator ===")
own = (x for x in [1, 2])
print(iter(own) is own)
print(next(own))
print(next(own))

print("=== StopIteration ===")
tiny = (x for x in [5])
print(next(tiny))
stopped = 0
try:
    next(tiny)
except StopIteration:
    stopped = 1
print(stopped)

print("=== break abandons it ===")
for v in (x for x in range(100)):
    if v == 2:
        break
    print(v)

print("=== over a list, tuple, string and dict ===")
for v in (x for x in [1, 2]):
    print(v)
for v in (x for x in (3, 4)):
    print(v)
for v in (c for c in "ab"):
    print(v)

print("=== globals are looked up late ===")
scale = 2
late = (x * scale for x in range(3))
scale = 10
for v in late:
    print(v)

print("=== outermost iterable is evaluated eagerly ===")
source = [1, 2]
eager = (x for x in source)
source = [9, 9, 9]
for v in eager:
    print(v)

print("=== mutating the captured iterable is visible ===")
shared = [1]
watcher = (x for x in shared)
shared.append(2)
for v in watcher:
    print(v)

print("=== inside a function ===")


def totals(items, bias):
    running = 0
    for v in (item + bias for item in items):
        running = running + v
    return running


print(totals([1, 2, 3], 10))


def counted(limit):
    return (x for x in range(limit) if x != 1)


for v in counted(4):
    print(v)

print("=== freevar snapshot (D-0046, differs from CPython) ===")


def snapshot():
    bias = 100
    held = (x + bias for x in [1, 2])
    bias = 900
    total = 0
    for v in held:
        total = total + v
    return total


print(snapshot())

print("=== generator expression feeding a generator function ===")


def scaled(source):
    for v in source:
        yield v * 3


for v in scaled(x for x in range(3)):
    print(v)

print("=== generator expression over a generator ===")


def base():
    yield 1
    yield 2
    yield 3


for v in (x * 10 for x in base()):
    print(v)

print("=== list comprehension nested inside ===")
rows = [[1, 2], [3, 4]]
for v in (len([c for c in row]) for row in rows):
    print(v)

print("=== conditional element ===")
for v in (x if x % 2 == 0 else -x for x in range(4)):
    print(v)

print("=== stored in a list and consumed later ===")
held = [(x for x in [1, 2]), (x for x in [3])]
for item in held:
    for v in item:
        print(v)

print("=== many steps ===")
count = 0
for v in (x for x in range(200) if x % 2 == 0):
    count = count + 1
print(count)
