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

print("=== 7. Break and Continue ===")
break_total = 0
break_index = 0
while break_index < 5:
    if break_index == 3:
        break
    break_total = break_total + break_index
    break_index = break_index + 1
print(break_total)

continue_total = 0
for continue_index in range(5):
    if continue_index == 2:
        continue
    continue_total = continue_total + continue_index
print(continue_total)

print("=== 8. Loop Else ===")
exhausted = 0
for value in range(2):
    exhausted = exhausted + value
else:
    exhausted = exhausted + 10
print(exhausted)

print("=== 9. Short-Circuit Logic ===")
print(0 and missing_name)
print(1 or missing_name)
print(2 and 3)
print(0 or 4)

print("=== 10. Zero-Argument Functions ===")
def get_function_value():
    return 42

function_value = get_function_value()
print(function_value)

print("=== 11. Function Parameters ===")
def add_values(left, right):
    return left + right

print(add_values(7, 5))

print("=== 12. Recursive Functions ===")
def fibonacci(number):
    if number < 2:
        return number
    return fibonacci(number - 1) + fibonacci(number - 2)

print(fibonacci(8))

print("=== Feature Test Complete ===")
