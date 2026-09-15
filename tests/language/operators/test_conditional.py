# Conditional expressions: then if condition else else.

print("=== true/false branches ===")
print("yes" if 1 else "no")
print("yes" if 0 else "no")
print("yes" if True else "no")
print("yes" if False else "no")
print("yes" if [] else "no")
print("yes" if [0] else "no")
print("yes" if "" else "no")
print("yes" if "x" else "no")
print("yes" if None else "no")
print(1 if 1 else 0)
print(1 if 0 else 0)

print("=== nesting ===")
print("a" if 1 else "b" if 1 else "c")
print("a" if 0 else "b" if 1 else "c")
print("a" if 0 else "b" if 0 else "c")
print(("a" if 1 else "b") if 0 else "c")

print("=== and/or/comparisons ===")
print(1 if 1 or 0 else 2)
print(1 if 0 or 0 else 2)
print(1 if 1 and 0 else 2)
print(1 if 1 and 1 else 2)
print(9 if 3 > 1 else 8)
print(9 if 3 < 1 else 8)
print("in" if "a" in "ab" else "out")
print("in" if "z" in "ab" else "out")
print("same" if None is None else "diff")
print(1 or 0 if 0 else 2)
print(0 or 5 if 1 else 2)
print(1 if 0 else 0 or 9)
print(not 0 if 1 else 1)

print("=== calls/lists/comprehensions ===")
print(len("ab" if 1 else "abcd"))
print([1 if 1 else 0, 1 if 0 else 2])
print([x if x % 2 else -x for x in range(4)])
print([x for x in range(4) if x])
print([x for x in range(3) if (1 if x else 0)])
print({1 if 1 else 0, 2} == {1, 2})

print("=== only one branch ===")
seen = [0]


def mark(n):
    seen[0] = seen[0] + 1
    return n


print(mark(1) if 1 else mark(2))
print(seen[0])
print(mark(3) if 0 else mark(4))
print(seen[0])

print("=== assignment/return ===")
v = "t" if 1 else "e"
print(v)


def pick(flag):
    return "yes" if flag else "no"


print(pick(1))
print(pick(0))
