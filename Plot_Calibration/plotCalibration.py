import numpy as np
import matplotlib.pyplot as plt
import json



fileName = "data/device2_cal_accelerometer"#"usageExample"
dataFile = fileName+ ".txt"
calibrationFile = fileName+"_Matrix.json"
imageFile = fileName + ".png"#"_raw.png"
title = "Magnetometer Calibration"#"Magnetometer Measurements"
units = "µT"




with open(calibrationFile, "r") as file:
    data = json.load(file)
A = np.array(data["A-1"])
b = np.array(data["b"])



rawData = np.genfromtxt(dataFile, delimiter="\t")
numbElem = len(rawData)
calibratedData = np.zeros((numbElem, 3), dtype="float")
for i in range(numbElem):
    currentElement = np.array([rawData[i, 0], rawData[i, 1], rawData[i, 2]])
    calibratedData[i, :] = A @ (currentElement - b)


minValue = min(np.min(rawData[~np.isnan(rawData)]), np.min(calibratedData[~np.isnan(calibratedData)]))
maxValue = max(np.max(rawData[~np.isnan(rawData)]), np.max(calibratedData[~np.isnan(calibratedData)]))
rangeValue = max(abs(maxValue), abs(minValue))*1.1


plt.style.use('ggplot')

fig, axs = plt.subplots(1, 3, figsize=(12, 4), sharey=True)

handles = [
    plt.Line2D([], [], color=f"C{0}", marker='.', linestyle='None', markersize=10, label='Raw Measurements'),
    plt.Line2D([], [], color=f"C{1}", marker='.', linestyle='None', markersize=10, label='Calibrated Measurements')
]
labels = [handle.get_label() for handle in handles]
fig.legend(handles, labels, loc='upper center', ncol=2, bbox_to_anchor=(0.5, 0.9375))
fig.suptitle(title)


# Plot XY data
axs[0].set_xlabel(f"X-axis [{units}]")
axs[0].set_ylabel(f"Y-axis [{units}]")
axs[0].grid(True)
axs[0].set_xlim(-rangeValue, rangeValue)
axs[0].set_ylim(-rangeValue, rangeValue)
axs[0].plot(rawData[:, 0], rawData[:, 1], '.', markersize=1, label="Raw Measurements")
axs[0].plot(calibratedData[:, 0], calibratedData[:, 1], '.', markersize=1, label="Calibrated Measurements")


# Plot YZ data
axs[1].set_xlabel(f"Y-axis [{units}]")
axs[1].set_ylabel(f"Z-axis [{units}]")
axs[1].grid(True)
axs[1].set_xlim(-rangeValue, rangeValue)
axs[1].set_ylim(-rangeValue, rangeValue)
axs[1].plot(rawData[:, 1], rawData[:, 2], '.', markersize=1, label="Raw Measurements")
axs[1].plot(calibratedData[:, 1], calibratedData[:, 2], '.', markersize=1, label="Calibrated Measurements")

# Plot XZ data
axs[2].set_xlabel(f"X-axis [{units}]")
axs[2].set_ylabel(f"Z-axis [{units}]")
axs[2].grid(True)
axs[2].set_xlim(-rangeValue, rangeValue)
axs[2].set_ylim(-rangeValue, rangeValue)
axs[2].plot(rawData[:, 0], rawData[:, 2],'.', markersize=1, label="Raw Measurements")
axs[2].plot(calibratedData[:, 0], calibratedData[:, 2], '.', markersize=1, label="Calibrated Measurements")



for ax in axs:
    ax.set_aspect('equal', adjustable='box')

x_major_locator = axs[0].xaxis.get_major_locator()
for ax in axs:
    ax.xaxis.set_major_locator(x_major_locator)
    ax.yaxis.set_major_locator(x_major_locator)


#plt.savefig(imageFile, dpi=300, bbox_inches='tight')
plt.show()