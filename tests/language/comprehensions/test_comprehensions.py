# List, set, and dict comprehensions. Generator expressions are rejected.

print("=== list basic ===")
print([x for x in range(4)])
print([x * x for x in range(5)])

print("=== empty iterable ===")
print([x for x in range(0)])
print([x for x in []])

print("=== filter ===")
print([x for x in range(6) if x])
print([x for x in range(5) if 0])
print([x * 2 for x in range(6) if x % 2 == 0])

print("=== nested for ===")
print([a * 10 + b for a in range(3) for b in range(2)])
print([a * 10 + b for a in range(3) if a != 1 for b in range(2) if b != 1])

print("=== nested listcomp ===")
rows = [[c for c in range(r)] for r in range(1, 4)]
print(rows[0])
print(rows[1])
print(rows[2])

print("=== iterate list ===")
src = [10, 20, 30]
print([n + 1 for n in src])

print("=== module name leak ===")
leaked = [n for n in range(3)]
print(leaked)
print(n)

print("=== function target is local ===")
n = 99


def collect():
    ys = [n for n in range(4)]
    return (ys, n)


print(collect()[0])
print(collect()[1])
print(n)

print("=== unbound local from comprehension ===")


def early_read():
    try:
        print(k)
    except NameError:
        print("unbound")
    result = [k for k in range(2)]
    return result


print(early_read())

print("=== outer name in elt ===")
base = 5
print([base + y for y in range(3)])

print("=== set comprehension ===")
print({x * x for x in range(1, 4)} == {1, 4, 9})
print(len({x for x in range(0)}))
print({x for x in range(6) if x % 2 == 0} == {0, 2, 4})
print({a + b for a in range(2) for b in range(2)} == {0, 1, 2})

print("=== dict comprehension ===")
d = {x: x * x for x in range(4)}
print(d[0])
print(d[3])
print(len(d))
print(len({k: k for k in range(0)}))
filtered = {x: x + 1 for x in range(6) if x % 2 == 1}
print(filtered[1])
print(filtered[5])
print(len(filtered))
nested = {a * 10 + b: a + b for a in range(2) for b in range(2)}
print(nested[0])
print(nested[11])
print(len(nested))

print("=== literals still parse ===")
print([1, 2, 3])
print({1, 2} == {2, 1})
print({"a": 1, "b": 2}["b"])

print("=== as call argument ===")
print(len([x for x in range(5)]))
print(list([x for x in range(3)]))
