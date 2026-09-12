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

print("=== Feature Test Complete ===")
