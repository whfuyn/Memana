from random import *

T = 1000
M = randint(2**31, 2**32)

n = 0

data = ''

for i in range(T + 1):
    k = randint(5, 30)
    for j in range(k + 1):
        t = randint(10, 100)
        case = '%d %d %d\n' % (i, t, randint(51234, 11234567))
        data += case
        n += 1

data = '%d %d\n' % (n, M) + data

with open('data\\input.txt', 'w') as f:
    f.write(data)
