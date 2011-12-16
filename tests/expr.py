# Test various expressions along with order of evaluation
def ev(r):
    print('r=%s' % r)
    return r

#def fn(*args, **kwargs):
#    print(args, kwargs)

class c:
    def __init__(self, n):
        self.n = ev(n)

ev(1) if ev(2) else ev(3)
ev(1) if ev(0) else ev(3)
ev(1) and ev(0)
ev(0) and ev(1)
ev(1) or ev(0)
ev(0) or ev(1)
ev(1), ev(2), ev(3), ev(4)
[ev(1), ev(2), ev(3), ev(4)]
(ev(1), ev(2), ev(3), ev(4))
{ev(1), ev(2), ev(3), ev(4)}

# XXX Python documentation is inconsistent with CPython behavior here
# {ev(1): ev(2), ev(3): ev(4)}
ev(1) + ev(2) * (ev(3) - ev(4))
#ev(fn)(ev(2), ev(3), *ev(4), **ev(5))

x = c(6)
y = c(7)
#ev(x).a, ev(y).a = ev(1), ev(2)
ev(x.n), ev(y.n)
