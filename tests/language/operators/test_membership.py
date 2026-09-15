# Membership operators and string for-iteration.

print("=== str in str ===")
print("th" in "Python")
print("x" in "Python")
print("" in "Python")
print("Python" in "Python")
print("on" not in "Python")
print("th" not in "Python")

print("=== list / tuple ===")
print(2 in [1, 2, 3])
print(9 in [1, 2, 3])
print(2 not in [1, 2, 3])
print("a" in ("a", "b"))
print("c" not in ("a", "b"))

print("=== dict / set ===")
d = {1: "one", 2: "two"}
print(1 in d)
print(3 in d)
print(2 not in d)
s = {10, 20, 30}
print(20 in s)
print(40 not in s)

print("=== if / while / expr ===")
if "e" in "hello":
    print("if-in-ok")
n = 0
while n < 3 and str(n) not in "12":
    print(n)
    n = n + 1
print(("x" in "xyz") and ("q" not in "xyz"))

print("=== for over string ===")
out = ""
for ch in "ab":
    out = out + ch
    print(ch)
print(out)

print("=== comprehension over string ===")
chars = [c for c in "xy"]
print(chars[0])
print(chars[1])
print(len(chars))
