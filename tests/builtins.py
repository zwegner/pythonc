for x in [bool, dict, enumerate, int, isinstance, len, list, open, ord, print, range, repr, reversed, set, sorted, str, tuple, zip]:
    print(x)

for x in [bool, dict, int, list, set, str, tuple]:
    print(x())

for x in [0, 1]:
    print(bool(x))

for x in [range(5)]:
    print(list(enumerate(x)))

for x in [0, 1, '0', '1', '10', '15']:
    print(int(x))

for x in ['', [], {}, set(), 'a', [1], {'b'}, {1: 2}]:
    print(len(x))

for x in [range(5), {1, 2, 3}, [1, 'c'], '', 'a', 'abc']:
    print(list(x))

for x in ['a', ' ', '+']:
    print(ord(x))

for x in [0, 1, 5, 10]:
    print(range(x))
    print(list(range(x)))

for x in [0, 1, [], {}, set(), '', 'a', '\n', '\r', '\t', '\\', '"', '"\'', '\'"', "'", (), (1,), (1, 2)]:
    print(repr(x))

for x in [[1], 'abc']:
    print(set(x))

for x in [[5, 3, 1]]:
    print(sorted(x))

for x in [[], range(1), range(5)]:
    print(tuple(x))

for x in [0, 'a', []]:
    print(str(x))

print(list(zip([1, 2], [3, 4])))
