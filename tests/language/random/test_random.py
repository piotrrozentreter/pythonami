import sys
sys.path.append("lib")
import random

random.seed(1)
print(random.getrandbits(8))
print(random.getrandbits(16))
print(random.randint(1, 6))
print(random.randrange(0, 10, 1))
print(random.randrange(0, 20, 3))
print(random.choice([10, 20, 30]))
print(random.choices([1, 2, 3], 5))
print(random.sample([1, 2, 3, 4, 5], 3))
xs = [1, 2, 3, 4]
random.shuffle(xs)
print(xs)
r = random.random()
print(r >= 0.0)
print(r < 1.0)
u = random.uniform(2.0, 5.0)
print(u >= 2.0)
print(u <= 5.0)
print(random.choices_weighted(["a", "b"], [1, 0], 4))

random.seed(1)
print(random.getrandbits(8))
