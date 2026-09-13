# Elif ladders, while-else, for-else, continue, nested loops.

print("=== elif ladder ===")
v = 0
if v == 1:
    print("a")
elif v == 0:
    print("b")
elif v == 2:
    print("c")
else:
    print("d")

v = 5
if v < 0:
    print("neg")
elif v == 0:
    print("zero")
else:
    print("pos")

print("=== while else completes ===")
i = 0
while i < 3:
    print(i)
    i = i + 1
else:
    print("while-ok")

print("=== while else skipped by break ===")
j = 0
while j < 5:
    j = j + 1
    if j == 2:
        break
    print(j)
else:
    print("should-not-print")
print("after-while-break")

print("=== for else ===")
for i in range(3):
    print(i)
else:
    print("for-else")

print("=== for continue ===")
for n in range(5):
    if n == 1:
        continue
    if n == 3:
        continue
    print(n)

print("=== nested for ===")
s = 0
for a in range(3):
    for b in range(2):
        s = s + a * 10 + b
print(s)

print("=== while continue ===")
k = 0
while k < 5:
    k = k + 1
    if k == 2:
        continue
    if k == 4:
        continue
    print(k)
