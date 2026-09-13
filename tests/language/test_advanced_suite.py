# Cross-cutting advanced suite: combines operators, control, functions,
# lists, strings, and builtins into one host/Amiga differential fixture.

print("=== 1. Floor division and modulo (negatives) ===")
print((-7) // 3)
print((-7) % 3)
print(7 // (-3))
print(7 % (-3))

print("=== 2. Short-circuit truthiness ===")
print([] and 1)
print([1] and 2)
print("" or "x")
print(None or 0 or 5)
print(not [])
print(not [1])

print("=== 3. Elif and while-else ===")
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

print("=== 4. Nested loops and continue ===")
total = 0
for a in range(3):
    for b in range(3):
        if b == 1:
            continue
        total = total + a * 10 + b
print(total)

print("=== 5. Recursion and list build ===")


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

print("=== 6. Strings and slices ===")
s = "abcdef"
print(s[1:4])
print(s + "!")
print(len(s))

print("=== 7. Range reverse and builtins ===")
acc = 0
for n in range(6, 0, -1):
    acc = acc + n
print(acc)
print(abs(-8))
print(min(-2, 3))
print(max(-2, 3))
print(bool(None))
print(int(True))

print("=== Advanced Suite Complete ===")
