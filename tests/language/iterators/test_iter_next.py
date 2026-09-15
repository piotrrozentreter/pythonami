# iter / next / StopIteration

print("=== list iter ===")
it = iter([1, 2, 3])
print(next(it))
print(next(it))
print(next(it))
caught = 0
try:
    next(it)
except StopIteration:
    caught = 1
print(caught)
print(next(it, 99))

print("=== tuple and string ===")
it2 = iter((7, 8))
print(next(it2))
print(next(it2, -1))
print(next(it2, -1))
chars = iter("ab")
print(next(chars))
print(next(chars))

print("=== for over iter ===")
total = 0
for x in iter([10, 20, 30]):
    total = total + x
print(total)

print("=== double iter identity ===")
base = iter([1])
same = iter(base)
print(same is base)
print(next(same))

print("=== errors ===")
bad = 0
try:
    iter(5)
except TypeError:
    bad = 1
print(bad)
bad = 0
try:
    next(5)
except TypeError:
    bad = 1
print(bad)
