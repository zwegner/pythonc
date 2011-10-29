def fib(x):
    if x <= 1:
        return 1
    return fib(x-1) + fib(x-2)

for i in range(15):
    print(fib(i))
