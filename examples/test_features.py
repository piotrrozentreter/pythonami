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

print("=== Feature Test Complete ===")
