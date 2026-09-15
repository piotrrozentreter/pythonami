# Example: importable random module (lib/random.py + lib/randgen.py)

import sys
sys.path.append("lib")
import random

random.seed(422)
print(random.randint(1, 100))
print(random.choice(["red", "green", "blue"]))
print(random.sample([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], 3))
