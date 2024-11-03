
import numpy as np
import matplotlib.pyplot as plt


dataFile = "data/device2_cal2_gyroscope.txt"
rawData = np.genfromtxt(dataFile, delimiter="\t")

cleanData = rawData[:, ~np.isnan(rawData).all(axis=0)]

columnAverages = np.mean(cleanData, axis=0)

print("Gyro Bias:", columnAverages)





# perform eye test to confirm data looks ok
plt.style.use('ggplot')
plt.plot(cleanData[:, 0])
plt.title('X Axis Data')
plt.xlabel('Index')
plt.ylabel('Value (deg/s)')



plt.plot(cleanData[:, 1])
plt.title('Y Axis Data')



plt.plot(cleanData[:, 2])
plt.title('Z Axis Data')
plt.legend(['X Axis', 'Y Axis', 'Z Axis'])
plt.show()

