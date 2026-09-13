# String concat, index, slice, len, and builtins that touch strings.

print("=== concat ===")
print("ab" + "cd")
print("Hi" + "!")
print("" + "x")
print("y" + "")

print("=== index and slice ===")
text = "Python68K"
print(text[0])
print(text[6])
print(text[0:6])
print(text[6:9])
print(text[1:5])
print(len(text))

print("=== empty and falsy ===")
print(len(""))
print(bool(""))
print(bool("a"))

print("=== str/int round trip ===")
print(str(7))
print(int("42"))
print(int("0"))
print(str(0))
print(str(True))
print(str(False))
