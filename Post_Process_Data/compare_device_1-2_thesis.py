2


import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
from scipy.signal import correlate

    
def match(df1, df2, match_start_from_end = 10, match_end_from_end = 5, fs=455):
    compare_time_start = df1["Timestamp"].iloc[-1] - match_start_from_end
    compare_time_stop = df1["Timestamp"].iloc[-1] - match_end_from_end
    
    lag_xyz = {}
    for axis in ["PosX", "PosY", "PosZ"]:
        data1 = df1[axis].to_numpy()
        data2 = df2[axis].to_numpy()
        
        data1 = (data1 - np.mean(data1)) / np.std(data1)
        data2 = (data2 - np.mean(data2)) / np.std(data2)
        
        correlation = correlate(data1, data2, mode='full')
        total_lag = np.argmax(correlation) - (len(data2) - 1)
        lag_xyz[axis] = total_lag
        
    #lag_xyz[axis] = total_lag
    #total_lag = np.mean(list(lag_xyz.values()))
    
    total_lag = lag_xyz["PosZ"]
    print (f"Total lag: {total_lag}")
    if(total_lag < 0):
        df1["Timestamp"] = df1["Timestamp"] - total_lag / fs
    else:
        df2["Timestamp"] = df2["Timestamp"] + total_lag / fs
    x_diff = df_device1["PosX"] - df_device2["PosX"]
    x_diff_mean = x_diff[(df_device1["Timestamp"] > compare_time_start) & (df_device1["Timestamp"] < compare_time_stop)].mean()

    y_diff = df_device1["PosY"] - df_device2["PosY"]
    y_diff_mean = y_diff[(df_device1["Timestamp"] > compare_time_start) & (df_device1["Timestamp"] < compare_time_stop)].mean()

    z_diff = df_device1["PosZ"] - df_device2["PosZ"]
    z_diff_mean = z_diff[(df_device1["Timestamp"] > compare_time_start) & (df_device1["Timestamp"] < compare_time_stop)].mean()

    df_device1["PosX"] = df_device1["PosX"] - x_diff_mean
    df_device1["PosY"] = df_device1["PosY"] - y_diff_mean
    df_device1["PosZ"] = df_device1["PosZ"] - z_diff_mean
    return df1, df2

def get_files(log, exercise, set_number):
    file_device1_base_path = Path("data/calibrated_device1_motion")
    file_device2_base_path = Path("data/calibrated_device2_motion")
    device1_data_file_names = [data_file.name for data_file in file_device1_base_path.glob("*.csv")]
    device2_data_file_names = [data_file.name for data_file in file_device2_base_path.glob("*.csv")]

    match_name_1 = f"device1_log{log}_calibrated_{exercise}_set{set_number}"
    match_name_2 = f"device2_log{log}_calibrated_{exercise}_set{set_number}"

    file_name_1 = ""
    for f in device1_data_file_names:
        if match_name_1 in f:
            file_name_1 = f
            break

    file_name_2 = ""
    for f in device2_data_file_names:
        if match_name_2 in f:
            file_name_2 = f
            break

    if file_name_1 == "" or file_name_2 == "":
        raise ValueError("File not found")

    file_device1_path = file_device1_base_path / file_name_1
    file_device2_path = file_device2_base_path / file_name_2
    return file_device1_path, file_device2_path




#dev1, dev2 = get_files(13, "b", 3)
dev1, dev2 = get_files(3, "b", 1)
df_device1 = pd.read_csv(dev1)
df_device2 = pd.read_csv(dev2)
df_device1, df_device2 = match(df_device1, df_device2)


plt.style.use("ggplot")
fig, axes = plt.subplots(nrows=3, sharex=True)# sharey=True)
fig.suptitle("World Positions")


axes[0].plot(df_device1["Timestamp"], df_device1["PosX"], label="PosX Device a")
axes[1].plot(df_device1["Timestamp"], df_device1["PosY"], label="PosY Device a")
axes[2].plot(df_device1["Timestamp"], df_device1["PosZ"], label="PosZ Device a")

axes[0].plot(df_device2["Timestamp"], df_device2["PosX"], label="PosX Device b")
axes[1].plot(df_device2["Timestamp"], df_device2["PosY"], label="PosY Device b")
axes[2].plot(df_device2["Timestamp"], df_device2["PosZ"], label="PosZ Device b")

for ax in axes:
    ax.legend(loc="upper left")
    ax.set_ylabel("Meters")
    ax.set_xlabel("Timestamp")

plt.show()