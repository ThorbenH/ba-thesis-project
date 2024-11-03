

import pandas as pd
import numpy as np
from pathlib import Path
import re

import matplotlib.pyplot as plt


def get_energie(device, log, exercise, set_number):
    file_device1_base_path = Path(f"data/calibrated_device{device}_motion")
    device1_data_file_names = [data_file.name for data_file in file_device1_base_path.glob("*.csv")]
    match_name_1 = f"device{device}_log{log}_calibrated_{exercise}_set{set_number}"
    file_name_1 = ""
    for f in device1_data_file_names:
        if match_name_1 in f:
            file_name_1 = f
            break
    if file_name_1 == "":
        return -1
    energy = int(re.search(r"energy(\d+)joule", file_name_1).groups()[0])
    return energy


def set_number_to_order(df):
    df = df.copy()
    
    set_numbers = sorted(df["set"].unique())
    
    df["set"] = df["set"].apply(lambda x: set_numbers.index(x))
    return df



devices = range(1, 6)
logs = range(1, 25)
exercises = ["b", "s", "o", "c"]
sets = range(1, 15)

data = []

for exercise in exercises:
    for device in devices:
        for s in sets:
            for log in logs:
                energy = get_energie(device, log, exercise, s)
                if energy != -1:
                    device_name = device-1
                    if device_name == 0:
                        device_name = "1 (Device a)"
                    elif device_name == 1:
                        device_name = "1 (Device b)"
                    data.append({"exercise": exercise, "user": str(device_name), "set": s, "energy": energy, "log": log})



df = pd.DataFrame(data)

plt.style.use("ggplot")
plt.figure(figsize=(10, 6))
colors = {"1 (Device a)": "C6", "1 (Device b)": "C1", "2": "C3", "3": "C5", "4": "C0"}



max_set_total = 0
for i, exercise in enumerate(exercises):
    subset = df[df["exercise"] == exercise]
    for user in subset["user"].unique():
        user_subset = subset[subset["user"] == user]
        for log in user_subset["log"].unique():
            user_log_subset = set_number_to_order(user_subset[user_subset["log"] == log])
            max_set_total = max(max_set_total, user_log_subset["set"].max())
max_set_total += 1#makes sure one set = 1 two sets = 2 (generted from incies so used to start at 0)


scale_x_base = 1
scale_exercise_column = 0.3

markers = ["s", "o", "D", "^", "x"]#, "v", "<", ">", "p", "P", "*", "h", "H", "+", "X", "d", "|", "_"]

for i, exercise in enumerate(exercises):
    subset = df[df["exercise"] == exercise]
    x_base = i * scale_x_base
    for j, user in enumerate(subset["user"].unique()):
        marker = markers[j]
        user_subset = subset[subset["user"] == user]
        for log in user_subset["log"].unique():
            user_log_subset = set_number_to_order(user_subset[user_subset["log"] == log])
            user_log_subset["x_pos"] = user_log_subset["set"] / max_set_total# possible values 0 just under 1
            user_log_subset["x_pos"] -= 0.5#possible values -0.5 just under 0.5
            user_log_subset["x_pos"] *= 2#possible values -1 just under 1
            user_log_subset["x_pos"] *= scale_exercise_column
            user_log_subset["x_pos"] += scale_exercise_column / 2
            user_log_subset["x_pos"] += x_base
            plt.scatter(user_log_subset["x_pos"], user_log_subset["energy"], color=colors[user], marker=marker)

ticks = []
tick_lables = []
for i, exercise in enumerate(exercises):
    x = i * scale_x_base
    ticks.append(x)
    if exercise == "b":
        tick_lables.append("Bench press")
    elif exercise == "s":
        tick_lables.append("Barbell back squat")
    elif exercise == "o":
        tick_lables.append("Overhead press")
    elif exercise == "c":
        tick_lables.append("Bicep curl")



plt.xticks(ticks, tick_lables)


for j, user in enumerate(df["user"].unique()):
    marker = markers[j]
    plt.scatter([], [], label=f"{user}", color=colors[user], marker=marker)

plt.xlabel("Exercise Type")
plt.ylabel("Energy [Joule]")
plt.title("Energy by Exercise Type")
plt.legend(title="User", loc = "upper right")
plt.show()
