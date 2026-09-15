# Fixed-count unpacking / multiple assignment (D-0038).

print("=== tuple source ===")
a, b = (1, 2)
print(a)
print(b)

print("=== list source ===")
c, d = [3, 4]
print(c)
print(d)

print("=== unparenthesized rhs ===")
e, f = 5, 6
print(e)
print(f)

print("=== trailing comma and swap ===")
g, = (7,)
print(g)
left = 1
right = 2
left, right = right, left
print(left)
print(right)

print("=== string source ===")
ch0, ch1 = "ab"
print(ch0)
print(ch1)

print("=== function locals ===")


def add_pair(pair):
    x, y = pair
    return x + y


print(add_pair((8, 9)))

print("=== for unpack ===")
total = 0
for first, second in [(1, 10), (2, 20)]:
    total = total + first + second
print(total)

print("=== parenthesized for target ===")
seen = 0
for (p, q) in ((3, 4),):
    seen = p + q
print(seen)

print("=== dict items ===")
only_k = None
only_v = None
for k, v in {"x": 7}.items():
    only_k = k
    only_v = v
print(only_k)
print(only_v)

print("=== length errors ===")
short = 0
try:
    too_few, extra = [1]
except ValueError:
    short = 1
print(short)
long = 0
try:
    one, two = [1, 2, 3]
except ValueError:
    long = 1
print(long)
bad_type = 0
try:
    u, v = 1
except TypeError:
    bad_type = 1
print(bad_type)
