import matplotlib.pyplot as plt
import numpy as np
import sys

fileName = sys.argv[1]

spaceSize = [-1, -1]
objects = [[],[]]
with open(fileName, "r", newline="") as f:
    while True:
        line = f.readline()
        if not line: break
        else:
            split = line.split(",")
            if not split: break
            else:
                objects[0].append(float(split[0]))
                objects[1].append(float(split[1]))
objects = np.array(objects)
print("The number of objects: " + str(len(objects[0])))

minLatitude = min(objects[0])
minLongitude = min(objects[1])
maxLatitude = max(objects[0])
maxLongitude = max(objects[1])

print("Minimum latitude/longitude:" + str(minLatitude) + "/" + str(minLongitude))
print("Maximum latitude/longitude:" + str(maxLatitude) + "/" + str(maxLongitude))

figsize = 40

plt.figure(figsize=(figsize,figsize))
plt.grid(True)
plt.scatter(objects[0], objects[1], marker=',', lw=0, s=2, color='black')

plt.xlabel('Latitude')
plt.ylabel('Longitude')
plt.axis([minLatitude, maxLatitude, minLongitude, maxLongitude])
plt.savefig(fileName+'.png', bbox_inches='tight')
