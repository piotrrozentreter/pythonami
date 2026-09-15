# Identity comparison operators is / is not.

print("=== None ===")
x = None
print(x is None)
print(x is not None)
print(None is None)
print(1 is None)
print(None is not 1)

print("=== bool ===")
print(True is True)
print(False is False)
print(True is False)
print(True is 1)
print(True == 1)
print(False is 0)
print(False == 0)

print("=== int ===")
print(1 is 1)
print(1 is 2)
print(1 is not 2)
a = 5
b = 5
print(a is b)
print(a is not b)

print("=== is not vs unary not ===")
print(0 is not 1)
print(0 is (not 1))

print("=== lists ===")
left = [1, 2]
right = [1, 2]
alias = left
print(left == right)
print(left is left)
print(left is alias)
print(left is right)
print(left is not right)

print("=== tuples ===")
t1 = (1, 2)
t2 = (1, 2)
print(t1 == t2)
print(t1 is t2)
print(t1 is not t2)

print("=== not (x is y) ===")
print(not x is None)
print(not 1 is None)
