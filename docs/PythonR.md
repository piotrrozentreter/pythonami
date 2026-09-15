# Python Random Module Functions

Python68K ships a Park–Miller generator in `lib/randgen.py` and the API below in
`lib/random.py`. Put `lib` on `sys.path`, then `import random`.

Language Level 0.6 adaptations vs CPython:

- No default arguments or kwargs: `randrange(start, stop, step)` always takes
  three arguments; weighted picks use `choices_weighted(population, weights, k)`.
- `seed(a)` requires a positive integer (no OS entropy / `None` seed).
- `getrandbits(k)` supports `1 <= k <= 31` (signed 32-bit ints).
- `shuffle(x)` has no optional `random` callable.

## 1. random.random()
Returns a random float number x such that 0.0 <= x < 1.0.

## 2. random.randint(a, b)
Returns a random integer N such that a <= N <= b.

## 3. random.randrange(start, stop[, step])
Returns a randomly selected element from the range created by start, stop, and step.
(Python68K: call `randrange(start, stop, step)` with an explicit step, usually `1`.)

## 4. random.uniform(a, b)
Returns a random floating-point number N such that a <= N <= b.

## 5. random.choice(sequence)
Returns a randomly selected element from a non-empty sequence.

## 6. random.choices(population, weights=None, *, cum_weights=None, k=1)
Returns a list of k elements chosen from the population with optional weights.
(Python68K: `choices(population, k)` for equal weights;
`choices_weighted(population, weights, k)` for weights.)

## 7. random.sample(population, k)
Returns a list of k unique elements chosen from the population.

## 8. random.shuffle(x[, random])
Shuffles the sequence x in place.
(Python68K: `shuffle(x)` only.)

## 9. random.seed(a=None, version=2)
Initializes the random number generator with a seed value a.
(Python68K: `seed(a)` with integer `a`.)

## 10. random.getrandbits(k)
Returns an integer with k random bits.
