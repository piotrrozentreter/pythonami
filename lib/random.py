# Python68K random module (docs/PythonR.md), layered on lib/randgen.py.
# Language Level 0.6 adaptations: no default args, no kwargs, no bitwise ops.
# Optional CPython parameters are expressed as separate entry points where needed.

import randgen


def seed(a):
    return randgen.seed(a)


def getrandbits(k):
    return randgen.getrandbits(k)


def random():
    return randgen.random()


def uniform(a, b):
    return a + (b - a) * randgen.random()


def randrange(start, stop, step):
    if step == 0:
        raise ValueError("zero step for randrange()")
    width = stop - start
    if step == 1:
        if width <= 0:
            raise ValueError("empty range for randrange()")
        return start + randgen.randbelow(width)
    if step > 0:
        n = (width + step - 1) // step
    else:
        n = (width + step + 1) // step
    if n <= 0:
        raise ValueError("empty range for randrange()")
    return start + step * randgen.randbelow(n)


def randint(a, b):
    return randrange(a, b + 1, 1)


def choice(sequence):
    n = len(sequence)
    if n == 0:
        raise IndexError("Cannot choose from an empty sequence")
    return sequence[randgen.randbelow(n)]


def choices(population, k):
    n = len(population)
    if n == 0:
        raise IndexError("Cannot choose from an empty population")
    if k < 0:
        raise ValueError("k must be non-negative")
    result = []
    i = 0
    while i < k:
        result.append(population[randgen.randbelow(n)])
        i = i + 1
    return result


def choices_weighted(population, weights, k):
    n = len(population)
    if n == 0:
        raise IndexError("Cannot choose from an empty population")
    if len(weights) != n:
        raise ValueError("weights length must match population")
    if k < 0:
        raise ValueError("k must be non-negative")
    total = 0
    i = 0
    while i < n:
        w = weights[i]
        if w < 0:
            raise ValueError("weights must be non-negative")
        total = total + w
        i = i + 1
    if total <= 0:
        raise ValueError("total of weights must be positive")
    result = []
    j = 0
    while j < k:
        r = randgen.random() * float(total)
        acc = 0
        i = 0
        picked = population[n - 1]
        while i < n:
            acc = acc + weights[i]
            if r < float(acc):
                picked = population[i]
                i = n
            else:
                i = i + 1
        result.append(picked)
        j = j + 1
    return result


def sample(population, k):
    n = len(population)
    if k < 0 or k > n:
        raise ValueError("sample larger than population or is negative")
    pool = list(population)
    result = []
    i = 0
    while i < k:
        j = randgen.randbelow(n - i)
        result.append(pool[j])
        pool[j] = pool[n - i - 1]
        i = i + 1
    return result


def shuffle(x):
    n = len(x)
    i = n - 1
    while i > 0:
        j = randgen.randbelow(i + 1)
        tmp = x[i]
        x[i] = x[j]
        x[j] = tmp
        i = i - 1
    return None
