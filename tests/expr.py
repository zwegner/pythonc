def side_effects(r):
    print('r=%s' % r)
    return r
for x in range(2):
    for y in range(2):
        for z in range(2):
            print(side_effects(x) if side_effects(y) else side_effects(z))
