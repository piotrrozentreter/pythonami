# Language Level 0.7 generators: yield, generator objects, for/iter/next.

print("=== simple counter ===")


def counter(n):
    i = 0
    while i < n:
        yield i
        i = i + 1


for v in counter(4):
    print(v)

print("=== laziness ===")


def noisy():
    print("start")
    yield 1
    print("middle")
    yield 2
    print("end")


g = noisy()
print("created")
print(next(g))
print(next(g))

print("=== next default and StopIteration ===")
g = counter(2)
print(next(g))
print(next(g))
print(next(g, -1))
print(next(g, -1))
stopped = 0
g2 = counter(1)
print(next(g2))
try:
    next(g2)
except StopIteration:
    stopped = 1
print(stopped)

print("=== bare yield ===")


def bare():
    yield
    yield 7


for v in bare():
    print(v)

print("=== generator is its own iterator ===")
it = counter(3)
print(iter(it) is it)
total = 0
for v in it:
    total = total + v
print(total)

print("=== arguments and locals ===")


def pairs(prefix, count):
    i = 0
    while i < count:
        yield prefix + str(i)
        i = i + 1


for text in pairs("row", 3):
    print(text)

print("=== early return ends the generator ===")


def early(stop):
    yield 1
    if stop:
        return
    yield 2


print("stop:")
for v in early(True):
    print(v)
print("run:")
for v in early(False):
    print(v)

print("=== break abandons the generator ===")


def three():
    yield 1
    yield 2
    yield 3


for v in three():
    if v == 2:
        break
    print(v)

print("=== partially consumed generator resumes ===")
shared = three()
print(next(shared))
rest = 0
for v in shared:
    rest = rest + v
print(rest)
for v in shared:
    print("never")
print("exhausted")

print("=== nested generators ===")


def inner():
    yield 1
    yield 2


def outer():
    for v in inner():
        yield v * 10
    yield 99


for v in outer():
    print(v)

print("=== yield inside for with else ===")


def with_else(n):
    for i in range(n):
        yield i
    else:
        yield -1


for v in with_else(2):
    print(v)

print("=== try / except / finally across yields ===")


def guarded():
    try:
        yield 1
        raise ValueError("boom")
    except ValueError:
        yield 2
    finally:
        yield 3


for v in guarded():
    print(v)

print("=== exception escapes the generator ===")


def raiser():
    yield 1
    raise ValueError("escapes")


caught = 0
try:
    for v in raiser():
        print(v)
except ValueError:
    caught = 1
print(caught)

print("=== generator over a list and a string ===")


def doubled(items):
    for item in items:
        yield item + item


for v in doubled([1, 2, 3]):
    print(v)
for v in doubled("ab"):
    print(v)

print("=== accumulating without materializing ===")


def squares(n):
    i = 0
    while i < n:
        yield i * i
        i = i + 1


acc = 0
for v in squares(5):
    acc = acc + v
print(acc)

print("=== many independent generators ===")
a = counter(2)
b = counter(3)
print(next(a))
print(next(b))
print(next(a))
print(next(b))
print(next(a, -1))
print(next(b))

print("=== generators in a list ===")
held = [counter(1), counter(2)]
for g in held:
    for v in g:
        print(v)

print("=== truthiness and identity ===")
one = counter(1)
print(one is one)
print(one is counter(1))

print("=== recursion depth is not consumed ===")


def deep(n):
    i = 0
    while i < n:
        yield i
        i = i + 1


count = 0
for v in deep(200):
    count = count + 1
print(count)

print("=== builtins that consume an iterable ===")
print(list(counter(4)))
print(tuple(counter(3)))
print(sorted(counter(3)))
print(sum(counter(5)))
print(sum(counter(3), 100))
print(len(list(counter(6))))
print(list(counter(0)))
print(sum(counter(0)))
print(any(v == 2 for v in counter(4)))
print(all(v < 4 for v in counter(4)))
print(any(v == 9 for v in counter(4)))
print(all(v == 0 for v in counter(4)))

print("=== consuming a partially stepped generator ===")
partial = counter(4)
print(next(partial))
print(list(partial))
print(list(partial))

print("=== a raise during collection propagates ===")
collected = 0
try:
    collected = list(raiser())
except ValueError:
    print("collect caught")
print(collected)

print("=== nested collection ===")
print(sum(sum(inner()) for i in range(2)))
print(list(sorted(counter(3))))
