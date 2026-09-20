# for-break, index assignment, and bool/None equality.

print("=== for break ===")
total = 0
for i in range(10):
    if i == 3:
        break
    total = total + i
print(total)

print("=== for break skips else ===")
marker = 0
for i in range(5):
    if i == 2:
        break
else:
    marker = 1
print(marker)

print("=== index assignment ===")
L = [10, 20, 30]
L[1] = 99
print(L[0])
print(L[1])
print(L[2])

print("=== nested index assignment ===")
G = [[1, 2], [3, 4]]
G[1][1] = 40
print(G[1][0])
print(G[1][1])

print("=== bool/None equality ===")
print(None == None)
print(None != None)
print(None == 0)
print(True == 1)
print(False == 0)
print(True != False)
print(True == False)
print(False != 1)

print("=== index augmented assignment ===")
L = [10, 20, 30]
L[1] += 5
print(L[1])
D = {}
D["n"] = 1
D["n"] += 2
print(D["n"])

print("=== ordering and sorted ===")
print("a" < "b")
print("b" < "a")
print(("a", 2) < ("b", 1))
print(("a", 1) < ("a", 2))
print(sorted([3, 1, 2]))
for letter, count in sorted({ "b": 2, "a": 1 }.items()):
    print("'" + letter + "':", count)

print("=== builtin arity diagnostics ===")
try:
    min(1)
    print("min: no error")
except TypeError:
    print("min: TypeError")
try:
    sorted([2, 1], 0)
    print("sorted: no error")
except TypeError:
    print("sorted: TypeError")
try:
    next()
    print("next: no error")
except TypeError:
    print("next: TypeError")
