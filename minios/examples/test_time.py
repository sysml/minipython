#import utime as time
import time

start = time.clock()
for x in range(0, 10000000):
    pass
end = time.clock()
elapsed = end - start
print("Time: " + str(elapsed))
