import imufusion
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import scipy
from scipy.spatial.transform import Rotation as R
import matplotlib.animation
import time
from filterpy.kalman import KalmanFilter
from scipy.signal import argrelextrema
import processing_settings as ps
from pathlib import Path
from tqdm import tqdm
import re


def ensureFloat64(df):
    for label in df.columns:
        if df[label].dtype == "object":
            df[label] = df[label].astype("float64")


def deltaTtoSecondsAndGenerateTimestamp(df):
    df["deltaT"] = df["deltaT"].astype("float64")
    df["deltaT"] = df["deltaT"] / 1000000
    df["timestamp"] = df["deltaT"].cumsum()
 
def generateRelativeHeightVelocity(df):
    df["relativeHeightVelocity"] = np.nan
    last_height = 0
    last_time = -0.1#ensure if first value exists we dont divide by 0
    for i, height in enumerate(df["relativeHeight"]):
        if not np.isnan(height):
            current_time = df["timestamp"].iloc[i]
            df.loc[i, "relativeHeightVelocity"] = (height - last_height) / (current_time - last_time)
            last_height = height
            last_time = current_time
 
def interpolateSporadicAndRenameBNO(df):
    for label in ["AbsQuatW", "AbsQuatX", "AbsQuatY", "AbsQuatZ", "linAccX", "linAccY", "linAccZ"]:
        df[label] = df[label].interpolate().bfill()
    df["plotRelativeHeight"] = df["relativeHeight"].interpolate().bfill()
    df.rename(columns={"AbsQuatW": "bnoQuatW", "AbsQuatX": "bnoQuatX", "AbsQuatY": "bnoQuatY", "AbsQuatZ": "bnoQuatZ"}, inplace=True)

def applyMadgwick(df, fs):
    timestep = df["deltaT"]
    gyroscope = df[["rotX", "rotY", "rotZ"]].to_numpy()
    accelerometer = df[["accX", "accY", "accZ"]].to_numpy() / 9.81 # madwick filter requires g
    quat = np.empty((len(df), 4))
    icm_rotation_matrices = np.empty((len(df), 3, 3))
    
    
    
    
    ahrs = imufusion.Ahrs()
    
    ahrs.settings = imufusion.Settings(
        imufusion.CONVENTION_NWU, 
        0.2,  # gain | A low gain will 'trust' the gyroscope more and so be more susceptible to drift. A high gain will increase the influence of other sensors (accelerometer and magnetometer) and reduce drift.
        250,  # gyroscope range (in degrees per second)
        1,  # acceleration rejection | Threshold (in degrees) used by the acceleration rejection feature
        10,  # magnetic rejection | same as above but for magnetometer
        5 * fs,  # recovery trigger period = 5 seconds | maximum continous time without acceleration/magnetic data before the rejection is reset
    )
    for index in range(len(df)):
        ahrs.update_no_magnetometer(gyroscope[index], accelerometer[index], timestep[index])  # 100 Hz sample rate
        quat[index] = ahrs.quaternion.wxyz
        icm_rotation_matrices[index] = ahrs.quaternion.to_matrix()
    df["icmQuatW"] = quat[:, 0] # do not trust these to be correct -> they dont work for acceleration -> use rotation matrix
    df["icmQuatX"] = quat[:, 1] # do not trust these to be correct -> they dont work for acceleration -> use rotation matrix
    df["icmQuatY"] = quat[:, 2] # do not trust these to be correct -> they dont work for acceleration -> use rotation matrix
    df["icmQuatZ"] = quat[:, 3] # do not trust these to be correct -> they dont work for acceleration -> use rotation matrix
    return icm_rotation_matrices

def localAccelerationToWorldFrameFromQuat(quaternions, vectors):
    outVectors = np.zeros([len(vectors), 3])
    for index in range(len(quaternions)):
        #length = np.linalg.norm(quaternions[index])
        #if abs(length-1.0) > 0.01:
        #    print("Quaternion norm not 1: ", length)        
        r = R.from_quat(quaternions[index])
        outVectors[index] = r.apply(vectors[index])
    return outVectors

def localAccelerationToWorldFrameFromMatrix(matrices_rotate, vectors):
    outVectors = np.zeros([len(vectors), 3])
    for index in range(len(matrices_rotate)):
        outVectors[index] = np.matmul(matrices_rotate[index], vectors[index])
    return outVectors

def icmLocalAcclerationsToWorldFrame(df):
    localAccelerationicm = df[["accX", "accY", "accZ"]].to_numpy()
    worldFrameAccelerationsicm = localAccelerationToWorldFrameFromMatrix(icm_rotation_matrices, localAccelerationicm)
    worldFrameAccelerationsicm[:, 2] -= 9.81#sbstract gravity to make linear accelerations
    df["icmWorldLinAccX"] = worldFrameAccelerationsicm[:, 0]
    df["icmWorldLinAccY"] = worldFrameAccelerationsicm[:, 1]
    df["icmWorldLinAccZ"] = worldFrameAccelerationsicm[:, 2]
    df["icmWorldLinAccMag"] = np.linalg.norm(worldFrameAccelerationsicm, axis=1)
    df["icmWorldLinAccMagXY"] = np.linalg.norm(worldFrameAccelerationsicm[:, :2], axis=1)


def bnoLocalAcclerationsToWorldFrame(df):
    localAccelerationBNO = df[["linAccX", "linAccY", "linAccZ"]].to_numpy()
    quaternionsBNO = df[["bnoQuatW", "bnoQuatX", "bnoQuatY", "bnoQuatZ"]].to_numpy()
    worldFrameAccelerationsBNO = localAccelerationToWorldFrameFromQuat(quaternionsBNO, localAccelerationBNO)
    df["bnoWorldLinAccX"] = worldFrameAccelerationsBNO[:, 0]
    df["bnoWorldLinAccY"] = worldFrameAccelerationsBNO[:, 1]
    df["bnoWorldLinAccZ"] = worldFrameAccelerationsBNO[:, 2]

def plotQuat(df):
    plt.style.use("ggplot")
    fig, axes = plt.subplots(nrows=4, sharex=True)
    axisLimit = 1.1
    fig.suptitle("Quaternion Bench 12 Repetitions")
    handles = [
        plt.Line2D([], [], color=f"C{0}", marker='.', linestyle='None', markersize=10, label='ICM-42688-P Madgwick'),
        plt.Line2D([], [], color=f"C{1}", marker='.', linestyle='None', markersize=10, label='BNO055')
    ]
    labels = [handle.get_label() for handle in handles]
    fig.legend(handles, labels, loc='upper center', ncol=2, bbox_to_anchor=(0.5, 0.95))
    #axes[0].plot(df["timestamp"], df["icmQuatW"], label="icm W")
    axes[0].plot(df["timestamp"], df["bnoQuatW"], label="BNO W")
    #axes[1].plot(df["timestamp"], df["icmQuatX"], label="icm X")
    axes[1].plot(df["timestamp"], df["bnoQuatX"], label="BNO X")
    #axes[2].plot(df["timestamp"], df["icmQuatY"], label="icm Y")
    axes[2].plot(df["timestamp"], df["bnoQuatY"], label="BNO Y")
    #axes[3].plot(df["timestamp"], df["icmQuatZ"], label="icm Z")
    axes[3].plot(df["timestamp"], df["bnoQuatZ"], label="BNO Z")
    axes[3].set_xlabel(f"Time [seconds]")
    for ax in axes:
        ax.legend(loc='upper right')
        ax.grid()
        ax.set_ylim(-axisLimit,axisLimit)

def integrate(df, columnName, deltaTColumnName):
    y = df[columnName]
    dt = df[deltaTColumnName]
    integrated = y*dt
    integrated = integrated.cumsum()
    return integrated

def derive(df, columnName, deltaTColumnName):
    y = df[columnName]
    dt = df[deltaTColumnName]
    derived = (y.diff() / dt).fillna(0)
    return derived

def highPassFilter2(df, fs, columnNames, cutoff=0.5, filter_order = 1):
    sos = scipy.signal.butter(filter_order, cutoff, 'highpass', fs=fs, output='sos')
    for columnName in columnNames:
        df[columnName] = scipy.signal.sosfiltfilt(sos, df[columnName])

def lowPassFilter2(df, fs, columnNames, cutoff=2, filter_order = 1):
    sos = scipy.signal.butter(filter_order, cutoff, 'lowpass', fs=fs, output='sos')
    for columnName in columnNames:
        df[columnName] = scipy.signal.sosfiltfilt(sos, df[columnName])

def applyStartTime(df, stabilization_time=2): # deprecated do not use
    df = df[df["timestamp"] >= stabilization_time]
    df = df.reset_index(drop=True)
    df["timestamp"] = df["timestamp"] - stabilization_time
    return df

def findAndApplyStopTime(df, fs, xy_acceleration_limit = 10, start_search__percentage = 0.75, end_accel_zeroing_time = 0.1): # deprecated do not use
    search_start = int(len(df) * start_search__percentage)
    stop_time = df.loc[search_start:].loc[df["icmWorldLinAccMagXY"] > xy_acceleration_limit, "timestamp"].iloc[0]
    df = df[df["timestamp"] <= stop_time]
    df = df.reset_index(drop=True)
    end_accel_zeroing_point = int(len(df) - end_accel_zeroing_time * fs)
    df.loc[end_accel_zeroing_point:, "icmWorldLinAccX"] = 0
    df.loc[end_accel_zeroing_point:, "icmWorldLinAccY"] = 0
    df.loc[end_accel_zeroing_point:, "icmWorldLinAccZ"] = 0
    return df

def applyKalmanFilter(df):
    dim_x = 2
    dim_z = 2
    kf = KalmanFilter(dim_x=dim_x, dim_z=dim_z) # dimx = state dimention, dimz = measurement dimention
    kf.x = np.array([0., 0.])  # initial state (location and velocity)
    kf.H = np.array([[1., 0.],
                     [0., 1.]])  # Measurement function
    kf.R = np.eye(dim_z) * 0.25# 0.05 works                                  0.04 -> see datasheet 0.4pa = 0.04m # measurement uncertainty
    kf.Q = np.eye(dim_x) * 0.0000008# 0.0000008 works                      #0.0065 -> see datasheet 0.6mg # process uncertainty
    filtered = np.zeros(len(df))
    for i in range(len(df)):#icmWorldDerivedAccZBetter       #icmWorldLinAccZ
        dt, u , z_pos, z_vel = df[["deltaT", "icmWorldDerivedAccZBetter", "relativeHeight", "relativeHeightVelocity"]].iloc[i]
        kf.F = np.array([[1., dt],  # state transition matrix
                        [0., 1.]])
        kf.B = np.array([0, dt]).T  # control transition matrix
        kf.predict(u = u)
        if not np.isnan(z_pos) and not np.isnan(u):
            kf.update([z_pos, z_vel])
        filtered[i] = kf.x[0]
    return filtered

def integrateToPositions(df, fs):
    df["bnoWorldLinVelX"] = integrate(df, "bnoWorldLinAccX", "deltaT")
    df["bnoWorldLinVelY"] = integrate(df, "bnoWorldLinAccY", "deltaT")
    df["bnoWorldLinVelZ"] = integrate(df, "bnoWorldLinAccZ", "deltaT")
    highPassFilter2(df, fs, ["bnoWorldLinVelX", "bnoWorldLinVelY", "bnoWorldLinVelZ"], 0.1, 2)
    df["bnoWorldPosX"] = integrate(df, "bnoWorldLinVelX", "deltaT")
    df["bnoWorldPosY"] = integrate(df, "bnoWorldLinVelY", "deltaT")
    df["bnoWorldPosZ"] = integrate(df, "bnoWorldLinVelZ", "deltaT")
    
    df["icmWorldLinVelX"] = integrate(df, "icmWorldLinAccX", "deltaT")
    df["icmWorldLinVelY"] = integrate(df, "icmWorldLinAccY", "deltaT")
    df["icmWorldLinVelZ"] = integrate(df, "icmWorldLinAccZ", "deltaT")
    highPassFilter2(df, fs, ["icmWorldLinVelX", "icmWorldLinVelY", "icmWorldLinVelZ"], 0.1, 2)
    df["icmWorldPosX"] = integrate(df, "icmWorldLinVelX", "deltaT")
    df["icmWorldPosY"] = integrate(df, "icmWorldLinVelY", "deltaT")
    df["icmWorldPosZ"] = integrate(df, "icmWorldLinVelZ", "deltaT")
    
    df["icmWorldDerivedAccZBetter"] = derive(df, "icmWorldLinVelZ", "deltaT")
    lowPassFilter2(df, fs, ["icmWorldDerivedAccZBetter"], 10, 1) #low pass filter as derivative is noisy (numeric differentiation and highpassfilter effects)
    df["kfPosZ"] = applyKalmanFilter(df)
    lowPassFilter2(df, fs, ["kfPosZ"], 2, 2)#kalman filter state update jumps -> low pass filter to smooth

def countReps(df, search_column,fs, peak_search_time = 0.5, was_inteligent_start_stop_time_used = False):
    n = int(fs*peak_search_time)
    data = df[search_column].to_numpy()
    
    min_name = f"min_{search_column}"
    max_name = f"max_{search_column}"
    repetition_name = f"repetition_{search_column}"
    motion_name = f"motion_{search_column}"
    
    df[min_name] = df.iloc[argrelextrema(data, np.less_equal, order=n)[0]][search_column]# only for plotting purposes
    df[max_name] = df.iloc[argrelextrema(data, np.greater_equal, order=n)[0]][search_column]# only for plotting purposes
    
    indices_min = argrelextrema(data, np.less_equal, order=n)[0]
    indices_max = argrelextrema(data, np.greater_equal, order=n)[0]
    extrema_indices = np.concatenate((indices_min, indices_max))
    extrema_indices.sort()
    extrema_distances = np.empty(0)
    for df_index in range(1, len(extrema_indices)):
        extrema_distances = np.append(extrema_distances, df[search_column][extrema_indices[df_index]] - df[search_column][extrema_indices[df_index-1]])
    
    mean_abs_distance = np.mean(np.abs(extrema_distances))
    
    
    exercise_rep_threshold = 1.2 * mean_abs_distance# some extrema show up due to the liftof period and noise -> this should filter them out
    if was_inteligent_start_stop_time_used:
        exercise_rep_threshold = 0.7 * mean_abs_distance
    
    
    
    current_rep = 0
    current_motion = 0
    rep_started = False# a rep consist of a sequence of two distances that are larger than the threshold
    
    
    
    df[repetition_name] = 0
    df[motion_name] = 0
    for df_index in range(len(df)):
        if df_index in extrema_indices:
            index_in_extrema_indices = np.where(extrema_indices == df_index)[0][0]
            if index_in_extrema_indices == len(extrema_indices) - 1: #extrema_distances has one less element than extrema_indices -> ignore last index
                pass
            elif abs(extrema_distances[index_in_extrema_indices]) > exercise_rep_threshold:
                if not rep_started:# a rep consist of a sequence of two distances that are larger than the threshold
                    current_rep += 1
                    rep_started = True
                else:# a rep consist of a sequence of two distances that are larger than the threshold
                    rep_started = False
                if extrema_distances[index_in_extrema_indices] > 0:
                    current_motion = 1
                else:
                    current_motion = -1
            else:
                current_motion = 0
        #df["motion"][df_index] = current_motion
        
        df.loc[df_index, motion_name] = current_motion
        if current_rep == 0:
            #df[][df_index] = -1# start time, not a valid rep
            df.loc[df_index, repetition_name] = -1
        elif current_motion == 0:
            #df["repetition"][df_index] = -2# rest time, not a valid rep
            df.loc[df_index, repetition_name] = -2
        else:
            #df["repetition"][df_index] = current_rep
            df.loc[df_index, repetition_name] = current_rep

def intelligentStartStopTime(df, fs):
    #print(f"Total time before intelligentStartStopTime: {df['timestamp'].iloc[-1]}")
    df["startStop_relative_Height_Helper"] = df["relativeHeight"].interpolate().bfill()
    lowPassFilter2(df, fs, ["startStop_relative_Height_Helper"], 2, 1)
    countReps(df, "startStop_relative_Height_Helper", fs, 0.5)
    
    start_time = df.loc[df["repetition_startStop_relative_Height_Helper"] == 1, "timestamp"].iloc[0]
    repetitions = df["repetition_startStop_relative_Height_Helper"].max()
    last_rep_last_index = df.loc[df["repetition_startStop_relative_Height_Helper"] == repetitions, "timestamp"].idxmax()
    stop_time = df.loc[last_rep_last_index, "timestamp"]
    
    df = df[df["timestamp"] <= stop_time]
    df = df[df["timestamp"] >= start_time]
    df["timestamp"] = df["timestamp"] - start_time
    df = df.reset_index(drop=True)
    #print(f"Total time after intelligentStartStopTime: {df['timestamp'].iloc[-1]}")
    return df
    
def calculateEnergyJoule(df, mass):
    energy = 0
    for rep in range(1, df["repetition_kfPosZ"].max()):
        repetitions = df.loc[df["repetition_kfPosZ"] == rep]
        start_height = repetitions.iloc[0]["kfPosZ"]
        middle_height = repetitions.loc[repetitions["motion_kfPosZ"].diff().fillna(0).ne(0).idxmax()]["kfPosZ"]#find index where motion changes motion[i] != motion[i+1] -> get that value
        end_height = repetitions.iloc[-1]["kfPosZ"]
        delta_height = (abs(start_height - middle_height) + abs(middle_height - end_height)) / 2
        rep_energy = 9.81 * delta_height * mass
        #print(f"Rep {rep} heights: Start: {start_height:.2f}, Middle: {middle_height:.2f}, End: {end_height:.2f}")
        #print(f"Rep {rep} delta_height: {delta_height:.2f} energy: {rep_energy:.2f}Joule")
        energy += rep_energy
   # print(f"Total energy: {energy:.2f}Joule")
    return energy
    
    
def plotWorldFrameThings(df, title, bnoAcc=False, bnoVel=False, bnoPos=False, icmAcc=False, icmVel=False, icmPosXY=False, icmPosZ=False, relativeHeight=False, kfPosZ=False, plotMotionRepetition=False):
    print("Plotting against the kaiser")
    plt.style.use("ggplot")
    nrows = 3
    if plotMotionRepetition:
        nrows = 5
    
    
    fig, axes = plt.subplots(nrows=nrows, sharex=True)#, sharey=True)
    
    fig.suptitle(title)
    
    if bnoAcc:
        axes[0].plot(df["timestamp"], df["bnoWorldLinAccX"], label="bnoWorldLinAccX")
        axes[1].plot(df["timestamp"], df["bnoWorldLinAccY"], label="bnoWorldLinAccY")
        axes[2].plot(df["timestamp"], df["bnoWorldLinAccZ"], label="bnoWorldLinAccZ")
    if bnoVel:
        axes[0].plot(df["timestamp"], df["bnoWorldLinVelX"], label="bnoWorldLinVelX")
        axes[1].plot(df["timestamp"], df["bnoWorldLinVelY"], label="bnoWorldLinVelY")
        axes[2].plot(df["timestamp"], df["bnoWorldLinVelZ"], label="bnoWorldLinVelZ")
    if bnoPos:
        axes[0].plot(df["timestamp"], df["bnoWorldPosX"], label="bnoWorldPosX")
        axes[1].plot(df["timestamp"], df["bnoWorldPosY"], label="bnoWorldPosY")
        axes[2].plot(df["timestamp"], df["bnoWorldPosZ"], label="bnoWorldPosZ")
        for ax in axes:
            ax.set_ylabel("Meters")
    if icmAcc:
        axes[0].plot(df["timestamp"], df["icmWorldLinAccX"], label="icmWorldLinAccX")
        axes[1].plot(df["timestamp"], df["icmWorldLinAccY"], label="icmWorldLinAccY")
        axes[2].plot(df["timestamp"], df["icmWorldLinAccZ"], label="icmWorldLinAccZ")
        axes[2].plot(df["timestamp"], df["icmWorldDerivedAccZBetter"], label="icmWorldDerivedAccZBetter")
    if icmVel:
        axes[0].plot(df["timestamp"], df["icmWorldLinVelX"], label="icmWorldLinVelX")
        axes[1].plot(df["timestamp"], df["icmWorldLinVelY"], label="icmWorldLinVelY")
        axes[2].plot(df["timestamp"], df["icmWorldLinVelZ"], label="icmWorldLinVelZ")
    
    min_range = min(df["icmWorldPosX"].min(), df["icmWorldPosY"].min(), df["kfPosZ"].min(), df["icmWorldPosZ"].min(), df["plotRelativeHeight"].min())
    max_range = max(df["icmWorldPosX"].max(), df["icmWorldPosY"].max(), df["kfPosZ"].max(), df["icmWorldPosZ"].max(), df["plotRelativeHeight"].max())
    range = max(abs(max_range), abs(min_range))*1.1
    
    if icmPosXY:
        axes[0].plot(df["timestamp"], df["icmWorldPosX"], label="PosX")#icmWorldPosX
        axes[1].plot(df["timestamp"], df["icmWorldPosY"], label="PosY")#icmWorldPosY
        axes[0].set_ylabel("Meters")
        axes[1].set_ylabel("Meters")
        axes[0].set_ylim(-range, range)
        axes[1].set_ylim(-range, range)
    if icmPosZ:
        axes[2].plot(df["timestamp"], df["icmWorldPosZ"], label="icmWorldPosZ")
        axes[2].set_ylabel("Meters")
    if relativeHeight:
        axes[2].plot(df["timestamp"], df["plotRelativeHeight"], label="relativeHeight")
    if kfPosZ:
        #axes[2].plot(df["timestamp"], df["kfPosZ"], label="PosZ")
        axes[2].plot(df["timestamp"], df["kfPosZ"], label="kfWorldPosZ", c='k')
        #axes[2].scatter(df["timestamp"], df['min_kfPosZ'], c='r')
        #axes[2].scatter(df["timestamp"], df['max_kfPosZ'], c='g')
        axes[2].set_ylabel("Meters")
        axes[2].set_ylim(-range, range)
    if plotMotionRepetition:
        axes[3].plot(df["timestamp"], df["motion_kfPosZ"], label="Motion")
        axes[3].set_ylabel("Motion\nSign")
        axes[4].plot(df["timestamp"], df["repetition_kfPosZ"], label="Repetition")
        axes[4].set_ylabel("Count")
    
    #axes[4].set_xlabel("Time [seconds]")
    axes[2].set_xlabel("Time [seconds]")
    for ax in axes:
        ax.legend(loc='upper right')



def downsample(df, factor):
    return df.iloc[::factor].reset_index(drop=True)

def animate3dPosition(df):# side effect -> downsample
    df = downsample(df, 15)#downsample to 30Hz (fs/15)
    
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    
    axisMax = max(df["icmWorldPosX"].max(), df["icmWorldPosY"].max(), df["kfPosZ"].max())
    axisMin = min(df["icmWorldPosX"].min(), df["icmWorldPosY"].min(), df["kfPosZ"].min())
    ax.set_xlim(axisMin, axisMax)
    ax.set_ylim(axisMin, axisMax)
    ax.set_zlim(axisMin, axisMax)

    my_path, = ax.plot([0], [0], [0], label="icm World Position")
    point, = ax.plot([0], [0], [0], 'o')
    
    #my_path2, = ax.plot([0], [0], [0], label="BNO World Position")
    #point2, = ax.plot([0], [0], [0], 'o')

    def update(num):
        point.set_data([df["icmWorldPosX"].iloc[num]], [df["icmWorldPosY"].iloc[num]])
        point.set_3d_properties(df["kfPosZ"].iloc[num])
        my_path.set_data(df["icmWorldPosX"].iloc[:num], df["icmWorldPosY"].iloc[:num])
        my_path.set_3d_properties(df["kfPosZ"].iloc[:num])
        
        #point2.set_data([df["bnoWorldPosX"].iloc[num]], [df["bnoWorldPosY"].iloc[num]])
        #point2.set_3d_properties(df["bnoWorldPosZ"].iloc[num])
        #my_path2.set_data(df["bnoWorldPosX"].iloc[:num], df["bnoWorldPosY"].iloc[:num])
        #my_path2.set_3d_properties(df["bnoWorldPosZ"].iloc[:num])
        return point, my_path#, point2, my_path2

    ani = matplotlib.animation.FuncAnimation(fig, update, frames=len(df), interval=1000/30, blit=True, repeat=True)
    #ani.save('icm_world_position.gif', writer='pillow', fps=30)
    plt.show()



#dataframe = pd.read_csv("data\calibrated_device1\device1_log3_calibrated_b_set1_weight60-00.csv", sep=",")


#data_file_path = "data\calibrated_device1\device1_log3_calibrated_o_set2_weight40-00.csv"
data_file_path = "data\calibrated_device1\device1_log3_calibrated_s_set3_weight60-00.csv"



weight = re.search(r"weight(\d+-\d+)", data_file_path).groups()[0]
weight_float = float(weight.replace("-", "."))
dataframe = pd.read_csv(data_file_path, sep=",")

ensureFloat64(dataframe)
deltaTtoSecondsAndGenerateTimestamp(dataframe)#Madwick filter requires format seconds
icm_rotation_matrices = applyMadgwick(dataframe, ps.sampling_frequency)
interpolateSporadicAndRenameBNO(dataframe)  #-> required to plot BNO/BMP data
generateRelativeHeightVelocity(dataframe)#required for kalman filter
icmLocalAcclerationsToWorldFrame(dataframe)
bnoLocalAcclerationsToWorldFrame(dataframe)  #-> required to plot BNO/BMP data
dataframe = intelligentStartStopTime(dataframe, ps.sampling_frequency)
integrateToPositions(dataframe, ps.sampling_frequency)
countReps(dataframe, "kfPosZ", ps.sampling_frequency, ps.peak_search_time, was_inteligent_start_stop_time_used = True)#!!!!!!!!!!!!!!!!! set was_inteligent_start_stop_time_used to True if you use it

reps = dataframe["repetition_kfPosZ"].max()
energy = int(calculateEnergyJoule(dataframe, weight_float)) #we dont need the decimal places -> the make up >1permill of the total energy



plotWorldFrameThings(dataframe, "Final computed world positions", icmPosXY=True, kfPosZ=True, plotMotionRepetition=False, icmPosZ = True, relativeHeight=True)#
#animate3dPosition(dataframe) # note downsampling side effect
plt.show()




