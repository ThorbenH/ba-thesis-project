
import pandas as pd
import numpy as np
from pathlib import Path
import re

def get_energies(log, exercise, set_number):
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
        return -1, -1

    
    energy_1 = int(re.search(r"energy(\d+)joule", file_name_1).groups()[0])
    energy_2 = int(re.search(r"energy(\d+)joule", file_name_2).groups()[0])
    return energy_1, energy_2


logs = [3, 5, 6, 8, 11, 13]
exercises = ["b", "s", "o", "c"]
sets = [1, 2, 3]



for exercise in exercises:
    energy_difference = []
    average_energy = []
    for log in logs:
        for set in sets:
            energy_1, energy_2 = get_energies(log, exercise, set)
            energy_difference.append(energy_1 - energy_2)
            average_energy.append(energy_1)
            average_energy.append(energy_2)
            print(f"For exercise {exercise}, log {log}, set {set}: Energy difference: {energy_1 - energy_2} Joules")
    average_energy = np.mean(average_energy)
    mean_energy_difference = np.mean(np.abs(energy_difference))
    std_energy_difference = np.std(np.abs(energy_difference))
    
    percentage_mean = mean_energy_difference / average_energy * 100
    percentage_std = std_energy_difference / average_energy * 100
    print("-------------------------------------------------------------------------------------------------")
    print(f"Exercise: {exercise}")
    print(f"Mean energy difference: {mean_energy_difference:.3f} Joules")
    print(f"Std energy difference: {std_energy_difference:.3f} Joules")
    print(f"Mean energy difference percentage: {percentage_mean:.2f} %")
    print(f"Std energy difference percentage: {percentage_std:.2f} %")
    print("-------------------------------------------------------------------------------------------------")







