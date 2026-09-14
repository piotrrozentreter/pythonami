caught = 0
try:
    1 // 0
except ZeroDivisionError:
    caught = 1
print(caught)
try:
    raise ValueError("nope")
except ValueError as e:
    print(1)
finally:
    print(2)
