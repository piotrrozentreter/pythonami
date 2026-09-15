# Park-Miller minimal standard LCG (Schrage), signed 32-bit safe.
# State lives in a one-element list because Language Level 0.6 has no `global`.

_MOD = 2147483647
_A = 16807
_Q = 127773
_R = 2836
_state = [1]


def seed(a):
    if a <= 0:
        a = 1
    a = a % (_MOD - 1)
    if a == 0:
        a = 1
    _state[0] = a
    return None


def _next():
    s = _state[0]
    hi = s // _Q
    lo = s % _Q
    s = _A * lo - _R * hi
    if s <= 0:
        s = s + _MOD
    _state[0] = s
    return s


def random():
    return float(_next() - 1) / float(_MOD - 1)


def getrandbits(k):
    if k < 1:
        raise ValueError("number of bits must be positive")
    if k > 31:
        raise ValueError("number of bits must be <= 31")
    bound = 1
    i = 0
    while i < k:
        bound = bound * 2
        i = i + 1
    limit = ((_MOD - 1) // bound) * bound
    while 1:
        r = _next() - 1
        if r < limit:
            return r % bound


def randbelow(n):
    if n <= 0:
        raise ValueError("n must be positive")
    limit = ((_MOD - 1) // n) * n
    while 1:
        r = _next() - 1
        if r < limit:
            return r % n
