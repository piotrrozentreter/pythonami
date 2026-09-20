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
# Python 3 primary name OSError; IOError is an alias for the same kind.
try:
    raise OSError("disk")
except IOError:
    print(3)
try:
    raise IOError("disk")
except OSError as e:
    print(4)
