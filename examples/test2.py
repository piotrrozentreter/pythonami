print("=== EDGE CASE TESTS ===")

# --------------------------------------------------
# Operator Precedence
# --------------------------------------------------

print("2 + 3 * 4 =", 2 + 3 * 4)          # 14
print("(2 + 3) * 4 =", (2 + 3) * 4)     # 20

print("10 - 2 - 3 =", 10 - 2 - 3)       # 5 (left associative)
print("100 / 10 / 2 =", 100 / 10 / 2)   # 5.0

print("2 * 3 + 4 * 5 =", 2 * 3 + 4 * 5) # 26

print("1 + 2 * 3 - 4 =", 1 + 2 * 3 - 4) # 3

# Unary operators
print("-5 + 10 =", -5 + 10)             # 5
print("-(2 + 3) =", -(2 + 3))           # -5

# Comparison precedence
print("3 > 2 and 2 > 1 =", 3 > 2 and 2 > 1)

# --------------------------------------------------
# Boolean Logic
# --------------------------------------------------

print(True and False)     # False
print(True or False)      # True
print(not False)          # True
print(not True == False)  # True

# --------------------------------------------------
# Nested Expressions
# --------------------------------------------------

x = (2 + 3) * (4 + 5)
print("x =", x)           # 45

y = ((1 + 2) * (3 + 4)) - ((5 + 6) / 11)
print("y =", y)

# --------------------------------------------------
# Variable Reassignment
# --------------------------------------------------

a = 10
print(a)

a = a + 5
print(a)

a = a * 2
print(a)

# --------------------------------------------------
# Function Calls in Expressions
# --------------------------------------------------

def add(a, b):
    return a + b

print(add(1, 2))
print(add(1, 2) * 10)
print(add(add(1, 2), add(3, 4)))

# --------------------------------------------------
# Deep Recursion Test
# --------------------------------------------------

def fact(n):
    if n <= 1:
        return 1
    return n * fact(n - 1)

print(fact(6))            # 720

# --------------------------------------------------
# List Access
# --------------------------------------------------

numbers = [10, 20, 30]

print(numbers[0])
print(numbers[1])
print(numbers[2])

# --------------------------------------------------
# Runtime Error Tests
# --------------------------------------------------

print("--- EXPECTED ERRORS BELOW ---")

# Division by zero
print(10 / 0)

# Undefined variable
print(unknown_variable)

# Invalid list index
print(numbers[10])

# Negative list index
print(numbers[-1])

# Type mismatch
print("abc" - "def")

# Wrong function arguments
print(add(1))

# Infinite recursion
def loop():
    return loop()

print(loop())

def check(value, expected):
    if value == expected:
        print("PASS")
    else:
        print("FAIL:", value, "expected", expected)

check(2 + 3 * 4, 14)
check((2 + 3) * 4, 20)
check(10 - 2 - 3, 5)
check(-(-5), 5)
check(add(10, 20), 30)
check(fact(5), 120)
