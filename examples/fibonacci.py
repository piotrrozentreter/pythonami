print("=== Fibonacci (recursive) ===")


def fibonacci(n):
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


i = 0
while i < 11:
    print(fibonacci(i))
    i = i + 1
