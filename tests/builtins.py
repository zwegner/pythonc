for x in [abs, bool, bytes, dict, enumerate, int, isinstance, iter, len, list, max, min, open, ord, print, range, repr, reversed, set, sorted, str, tuple, type, zip]:
    print(x)

for x in [bool, bytes, dict, int, list, set, str, tuple]:
    print(x.__name__)
    print(x())
    print(x().__class__)

for x in [-1, 0, 1]:
    print(abs(x))

for x in [0, 1]:
    print(bool(x))

for x in [[], range(256), 0, 1, 10]:
    print(bytes(x))

for x in [[(1,2)]]:
    print(dict(x))

for x in [range(5)]:
    print(list(enumerate(x)))
print(enumerate([]).__class__)

for x in [0, 1, False, True, '0', '1', '10', '15', '-123', '+714']:
    print(int(x))

for x in [('10', 16), ('ff', 16), ('-abc', 36)]:
    print(int(*x))

for x in [[1], (2,), {3: 4}]:
    print(list(iter(x)))

for x in ['', [], {}, set(), 'a', [1], {'b'}, {1: 2}]:
    print(len(x))

for x in [range(5), {1, 2, 3}, [1, 'c'], '', 'a', 'abc']:
    print(list(x))

for x in [(1,), [2, 3], {4, 5, 6}]:
    print(max(x))
    print(min(x))

for x in ['a', ' ', '+']:
    print(ord(x))

for x in [0, 1, 5, 10]:
    print(range(x))
    print(list(range(x)))
print(range(5).__class__)

for x in [0, 1, [], {}, set(), '', 'a', '\n', '\r', '\t', '\\', '"', '"\'', '\'"', "'", (), (1,), (1, 2)]:
    print(repr(x))

for x in [[], [1], [1, 2, 3], (1, 2, 3)]:
    print(list(reversed(x)))

for x in [[1], 'abc']:
    print(set(x))

for x in [[5, 3, 1], (1, 3, 2), {10, 5, 0}]:
    print(sorted(x))

for x in [[], range(1), range(5)]:
    print(tuple(x))

for x in [0, False, None, int, type]:
    print(type(x))

for x in [0, False, True, None, 'a', []]:
    print(str(x))

print(list(zip([1, 2], [3, 4])))
print(list(zip([1, 2, 3], [4, 5])))
print(list(zip([1, 2,], [3, 4, 5])))
print(zip([], []).__class__)

x = {x: x*x for x in range(10)}
print(x.get(0, 5))
print(x.get(10, 20))
print(x.get(-1, 'a'))
print(x)
print(x.keys())
print(x.values())
print(x.items())
print(sorted(x))
print(sorted(x.keys()))
print(sorted(x.values()))
print(list(x.items()))
print(len(x))
print(len(x.keys()))
print(len(x.values()))
print(len(x.items()))

x = []
x.append(1)
x.extend([2, 3])
print(x)

x = set()
x.add(1)
x.update([2, 3])
print(x)

for x in [[], ['a'], ['a', 'b', 'c'], ('x', 'y', 'z')]:
    print(','.join(x))
for x in ['', 'a', ':', 'a:', 'a:b', 'a:b:c']:
    print(x.split(':'))
