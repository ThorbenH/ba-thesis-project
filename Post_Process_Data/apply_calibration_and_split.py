import numpy as np
import json
import pandas as pd
from pathlib import Path
from io import StringIO
import time
import re
import processing_settings as ps
import matplotlib.pyplot as plt

#delete columns by condition
#df = df[[c for c in df.columns if c not in "magX,magY,magZ,absHeight,deltaT"]]
#df.to_csv(fileName, index=False)
#barPresent,handPresent,batteryVoltage,currentPage,currentWeight,linAccX,linAccY,linAccZ,AbsQuatW,AbsQuatX,AbsQuatY,AbsQuatZ,accX,accY,accZ,rotX,rotY,rotZ,temp,magX,magY,magZ,absHeight,deltaT




def readCalibrationData(calibration_file_location):
    with open(calibration_file_path, "r") as file:
        data = json.load(file)
    A_inv = np.array(data["A-1"])
    b = np.array(data["b"])
    gyro_bias = np.array(data["gyroBias"])
    return A_inv, b, gyro_bias

def readCSV(fileName):
    text = Path(fileName).read_text()
    text = text.replace("x", "NaN")# Ensure columns are read as floats not objects
    buffer = StringIO(text)
    df = pd.read_csv(buffer, sep=",")
    return df

def calibrateData(df, A_inv, b, gyro_bias):
    df["rotX"] -= gyro_bias[0]
    df["rotY"] -= gyro_bias[1]
    df["rotZ"] -= gyro_bias[2]
    
    df["accX"] -= b[0]
    df["accY"] -= b[1]
    df["accZ"] -= b[2]
    df[["accX", "accY", "accZ"]] = np.matmul(df[["accX", "accY", "accZ"]], A_inv)

def isolateIfIsExercising(df, fs, seconds_before_exercise):
    df["isExercising"] = df["barPresent"] & df["handPresent"] & (df["currentPage"] != 'u') # 'u' is the page that displays device information in case it is lost and is not an exercise page
    # Adding some time before the excersize starts -> give the Orientation Filter (Madgwick) enough time to "look at" the gravity vector
    df["isExercising"] = df["isExercising"].rolling(seconds_before_exercise*fs, min_periods=1).apply(lambda x: x.any(), raw=True).shift(-(seconds_before_exercise*fs - 1)).fillna(False).astype(bool)

def addSetNumber(df):
    setNumbers = []
    setNumber = 0
    wasExerciseing = False #(setNumber == 0 is not a valid set)
    for isExercising in df["isExercising"]:
        if isExercising and not wasExerciseing:
            setNumber += 1
        setNumbers.append(setNumber)
        wasExerciseing = isExercising
    df["setNumber"] = setNumbers

def splitByExercise(df, outputFileBasePath, fs):
    dataFrames = []
    outputFileNames = []
    lastExercise = None
    exerciseSetNumber = 1
    for setNumber in df["setNumber"].unique():
        if(setNumber == 0):
            continue
        
        df_set = df[(df["setNumber"] == setNumber) & (df["isExercising"])]
        
        if len(df_set) < ps.seconds_before_exercise * fs+5:#+5 just to make sure its over, its late and i am tired i just want this to work
            print(f"Set {setNumber} is too short, skipping ------------------- BIG ERROR THIS SHOULD NOT HAPPEN")
            continue #this error shows up for ONE file and I have no idea why
        
        currentWeight = df_set["currentWeight"].values[seconds_before_exercise*fs] # weight is constant during exercise
        currentExercise = df_set["currentPage"].values[seconds_before_exercise*fs] # offset by prepended time since we want the value when the exercise starts
        if currentExercise == lastExercise:
            exerciseSetNumber += 1
        else:
            exerciseSetNumber = 1
        lastExercise = currentExercise
        
        weightString = f"{currentWeight:.2f}".replace(".", "-")
        outputFileName = f"{outputFileBasePath}_{currentExercise}_set{exerciseSetNumber}_weight{weightString}"
        df_set = df_set.drop(columns=["setNumber", "isExercising", "barPresent", "handPresent", "currentPage","batteryVoltage","currentPage","currentWeight", "temp"])
        df_set.reset_index(drop=True, inplace=True)
        dataFrames.append(df_set)
        outputFileNames.append(outputFileName)
    return dataFrames, outputFileNames

def absToRelativeHeight(df_s):
    for dataFrame in  df_s:
        firstHeight = [value for value in dataFrame["absHeight"] if not pd.isna(value)][0]
        dataFrame["relativeHeight"] = dataFrame["absHeight"] - firstHeight
        dataFrame = dataFrame.drop(columns=["absHeight"])

def saveDataFrames(dataFrames, output_file_names):
    for i, dataFrame in enumerate(dataFrames):
        dataFrame.to_csv(f"{output_file_names[i]}.csv", index=False)
        #dataFrame.to_parquet(f"{output_file_names[i]}.parquet", index=False)#save 94% speed and 75% storage

def saveRotationPlot(dataFrames, output_file_names, outputFileBasePath, fs, seconds_before_exercise):
    plt.style.use('ggplot')
    plot_limit = fs * (seconds_before_exercise + 5)
    fig, axes = plt.subplots(nrows=len(dataFrames), sharex=True)#, sharey=True)
    
    for i, ax in enumerate(axes):
        ax.set_title(f"{output_file_names[i]}")
        ax.plot(dataFrames[i].index[:plot_limit], dataFrames[i]["rotX"][:plot_limit], label="X")
        ax.plot(dataFrames[i].index[:plot_limit], dataFrames[i]["rotY"][:plot_limit], label="Y")
        ax.plot(dataFrames[i].index[:plot_limit], dataFrames[i]["rotZ"][:plot_limit], label="Z")
        ax.set_ylim(-20, 20)
        ax.legend(loc = "upper left")
        ax.set_ylabel(f"Degrees per second")
        ax.set_xlabel(f"Index (500 sample ~ 1 second)")
    fig.set_size_inches(10, 35)
    fig.tight_layout()
    fig.savefig(f"{outputFileBasePath}_rotations.png")
    plt.close(fig)
    #plt.clf()

def smoothenHandBar(df, fs, smoothening_time = 4):
    window_samples = smoothening_time * fs
    df["barPresent"] = df["barPresent"].rolling(window_samples, min_periods=1, center = True).apply(lambda x: x.any(), raw=True).fillna(False).astype(bool)
    df["handPresent"] = df["handPresent"].rolling(window_samples, min_periods=1, center = True).apply(lambda x: x.any(), raw=True).fillna(False).astype(bool)
    return df








data_file_name = "device3_log4.csv"
calibration_file_name = "cal_matrix.json"












data_file_path = Path("data")/data_file_name
calibration_file_path = Path("data")/calibration_file_name

device_number, log_number = re.search(r"device(\d+)_log(\d+).csv", data_file_name).groups()

output_dir = Path(f"data/calibrated_device{device_number}")
output_dir.mkdir(exist_ok=True)
output_file_base_path = output_dir/f"device{device_number}_log{log_number}_calibrated"

script_start_time = time.time()
seconds_before_exercise = ps.seconds_before_exercise


A_inv, b, gyro_bias = readCalibrationData(calibration_file_path)
data_frame = readCSV(data_file_path)


#data_frame = smoothenHandBar(data_frame, ps.sampling_frequency) # new because user error?


calibrateData(data_frame, A_inv, b, gyro_bias)
isolateIfIsExercising(data_frame, ps.sampling_frequency, seconds_before_exercise)
addSetNumber(data_frame)
data_frame_s, output_file_names = splitByExercise(data_frame, output_file_base_path, ps.sampling_frequency)

remove_indices = []
for i in range(len(data_frame_s)):
    if len(data_frame_s[i]) < ps.min_exercise_time * ps.sampling_frequency:#file has to be at least min_exercise_time seconds long 
        remove_indices.append(i)
for i in reversed(remove_indices):
    del data_frame_s[i]
    del output_file_names[i]


absToRelativeHeight(data_frame_s)
saveDataFrames(data_frame_s, output_file_names)
saveRotationPlot(data_frame_s, output_file_names, output_file_base_path, ps.sampling_frequency, seconds_before_exercise)

print(f"Time taken: {time.time() - script_start_time} seconds")