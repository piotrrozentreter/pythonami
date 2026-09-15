# Restricted f-strings (D-0043)

print("=== literals ===")
print(f"hi")
print(f"")
print(f"{{ok}}")
print(f"a{{b}}c")

print("=== expressions ===")
x = 42
print(f"x={x}")
print(f"a{1 + 2}b")
name = "Ada"
print(f"Hello, {name}!")

print("=== conversions ===")
s = "A\nB"
print(f"{s!r}")
print(f"{s!a}")
print(f"{x!s}")

print("=== format specs ===")
print(f"{x:04d}")
print(f"{7:d}")
