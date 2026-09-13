# Recursion, local shadowing, globals from functions, augassign in locals.

print("=== recursion fibonacci ===")


def fibonacci(n):
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


print(fibonacci(10))

print("=== factorial ===")


def fact(n):
    if n <= 1:
        return 1
    return n * fact(n - 1)


print(fact(6))

print("=== deep sum ===")


def deep(n):
    if n == 0:
        return 0
    return n + deep(n - 1)


print(deep(20))

print("=== local shadows global ===")
g = 100


def f(g):
    return g + 1


print(f(5))
print(g)

print("=== read global from function ===")
counter = 0


def tick():
    return counter


print(tick())
counter = 5
print(tick())

print("=== filter even sum ===")


def sum_evens(n):
    acc = 0
    for i in range(n):
        if i % 2 == 0:
            acc = acc + i
        else:
            continue
    return acc


print(sum_evens(10))

print("=== augassign in locals ===")


def bump(x):
    x += 1
    x *= 2
    x //= 2
    x -= 1
    x %= 3
    return x


print(bump(4))

print("=== multi return paths ===")


def choose(x):
    if x < 0:
        return abs(x)
    elif x == 0:
        return 0
    else:
        return x


print(choose(-4))
print(choose(0))
print(choose(9))

print("=== implicit None return ===")


def noop():
    x = 1


print(noop())
