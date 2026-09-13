# List concat, append/pop, nested index, range edges, len.

print("=== list concat ===")
A = [1, 2]
B = [3, 4]
print(A + B)
print(A)
print(B)

print("=== list_append / list_pop ===")
grown = [1, 2]
list_append(grown, 3)
list_append(grown, 4)
print(grown)
print(len(grown))
print(list_pop(grown))
print(grown)
print(len(grown))

print("=== nested list index ===")
G = [[1, 2], [3, 4], [5, 6]]
print(G[0][1])
print(G[2][0])
print(len(G))

print("=== build list in loop ===")


def build(n):
    out = []
    for i in range(n):
        list_append(out, i * i)
    return out


print(build(5))

print("=== range edges ===")
empty_count = 0
for x in range(0):
    empty_count = empty_count + 1
print(empty_count)

empty2 = 0
for x in range(2, 2):
    empty2 = empty2 + 1
print(empty2)

print("=== range step ===")
step_sum = 0
for x in range(1, 10, 2):
    step_sum = step_sum + x
    print(x)
print(step_sum)

print("=== range reverse ===")
rev = 0
for x in range(10, 0, -2):
    rev = rev + x
    print(x)
print(rev)

print("=== list slice ===")
L = [10, 20, 30, 40, 50]
print(L[1:4])
print(L[0:2])
print(L[2:5])
