x = 0
y = 4
while x < 1000000:
    x += 1
    y |= y * 7
    y &= 0xFFFFFFFF
print(x, y)

x = 0
y = 4
while x < 1000000:
    x += 1
    y = {x: y | y * 7}[x]
    y &= 0xFFFFFFFF
print(x, y)
