print("=== Python Interpreter Test ===")

# Variables and arithmetic
a = 10
b = 3

print("a =", a)
print("b =", b)
print("a + b =", a + b)
print("a - b =", a - b)
print("a * b =", a * b)
print("a / b =", a / b)

# Strings
name = "Python"
print("Hello " + name)

# Conditionals
if a > b:
    print("a is greater than b")
else:
    print("a is not greater than b")

# Loop
sum = 0
i = 1
while i <= 5:
    sum = sum + i
    i = i + 1

print("Sum 1..5 =", sum)

# Function
def square(x):
    return x * x

print("square(7) =", square(7))

# Recursion
def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n - 1)

print("factorial(5) =", factorial(5))

# List
numbers = [1, 2, 3, 4, 5]

print("List length =", len(numbers))
print("First item =", numbers[0])
print("Last item =", numbers[4])

i = 0
while i < len(numbers):
    print("numbers[", i, "] =", numbers[i])
    i = i + 1

# Boolean logic
flag = (a > 5) and (b < 5)
print("flag =", flag)

print("=== Test Complete ===")