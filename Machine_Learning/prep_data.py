from pathlib import Path
import pandas as pd
import re
from tqdm import tqdm
import json


#fs                                     : sampling frequency
#context_length_seconds                 : time over which a prediction is made
#downsampling_step_size                 : downsampling step size
#augmentation_step_size                 : step size between augmentations
def appendData(df_combined, input_file_path, fs = 455, context_length_seconds = 1.2, downsampling_step_size = 10, augmentation_step_size = 3, add_weight = False):
    df_raw_data = pd.read_csv(input_file_path)
    df_raw_data.drop(columns=["Motion","Repetition","Timestamp"], inplace=True)
    
    df_output = pd.DataFrame()
    df_slices = []
    
    slice_length = int(context_length_seconds * fs)
    slice_length -= slice_length % downsampling_step_size # make sure slice length is a multiple of downsampling step size
    slice_length += downsampling_step_size-1 # ensure enough "space" for downsampling steps | -1 so first step does not get an additional line
    
    # starting at the end and going backwards as incomplete slices are discarded and end of data is more important than start
    # (-> intelligentStartStop is better at detecting end then start, so there is more "trash" data at start)
    for start in range(len(df_raw_data) - slice_length, 0, -slice_length):
        end = start + slice_length
        df_slices.append(df_raw_data.iloc[start:end].reset_index(drop=True))
    
    augmentation_count = max(1, int(downsampling_step_size / augmentation_step_size))
    for df_slice in df_slices:
        for i in range(0, augmentation_count):
            offset = i * augmentation_step_size
            df_slice_downsampled_augmented = df_slice.iloc[offset::downsampling_step_size, :].reset_index(drop=True)
            df_slice_downsampled_augmented = df_slice_downsampled_augmented.stack().to_frame().T #flatten to one row
            df_output = pd.concat([df_output, df_slice_downsampled_augmented], ignore_index=True)
    _, _, exercise_type, _, weight_string, _, _ = re.search(r"device(\d+)_log(\d+)_calibrated_([bsoc])_set(\d+)_weight(\d+-\d+)_reps(\d+)_energy(\d+)joule", input_file_path.name).groups()
    weight = float(weight_string.replace("-", "."))
    
    df_output.columns = [f"{a}_{b}" for a, b in df_output.columns]#combines multi-indexed columns
    if add_weight:
        df_output["weight"] = weight
    
    
    with open("convert_exercises.json", "r") as f:
        exercise_to_index = json.load(f)["exercise_to_index"]
    df_output["exercise_type"] = exercise_to_index[exercise_type]
    
    #df_output["exercise_type"] = exercise_to_index[exercise_type]
    
    if df_combined is None:
        df_combined = df_output
    else:
        df_combined = pd.concat([df_combined, df_output], ignore_index=True)
    return df_combined





context_length_seconds = 1
downsampling_step_size = 15
augmentation_step_size = 20
training_ratio = 0.7
input_folder_base_path = Path("../Post_Process_Data/data/")


if downsampling_step_size % augmentation_step_size == 0:
    print("--------------------ERROR--------------------")
    print("- augmentation_step_size MUST NOT BE A DIVISOR OF downsampling_step_size")
    print("--------------------ERROR--------------------")
    exit()

file_paths = []
input_folder_path_s = [folder_path for folder_path in input_folder_base_path.iterdir() if folder_path.name.endswith("_motion") and folder_path.is_dir()]
for folder_path in input_folder_path_s:
    file_paths += [file_path for file_path in folder_path.iterdir() if file_path.name.endswith(".csv") and file_path.is_file()]

df_combined = None
for i in tqdm(range(0, len(file_paths))):
    df_combined = appendData(df_combined, file_paths[i], fs = 455,
                             context_length_seconds=context_length_seconds,
                             downsampling_step_size=downsampling_step_size,
                             augmentation_step_size=augmentation_step_size)

output_file_path = Path(f"data/train_data_contextMSeconds{int(context_length_seconds*1000)}_downsamplingStepSize{downsampling_step_size}_augmentationStepSize{augmentation_step_size}")


#df_combined.to_csv(f"{output_file_path}.csv", index=False)
#exit()


df_combined = df_combined.sample(frac=1).reset_index(drop=True)#shuffle so dataset is mixed an no exercise or user is overrepresented at any section
training_end_index = int(len(df_combined) * training_ratio)

df_train = df_combined.iloc[:training_end_index]
df_test = df_combined.iloc[training_end_index:]


df_train.to_parquet(f"{output_file_path}_train.parquet", index=False)
df_test.to_parquet(f"{output_file_path}_test.parquet", index=False)