
import time
from pathlib import Path
from model import get_model_and_dataset, load_latest_checkpoint_to_model
import pandas as pd
import numpy as np
from matplotlib import pyplot as plt
from sklearn.metrics import ConfusionMatrixDisplay, accuracy_score, confusion_matrix
import json

start_time = time.time()
import tensorflow as tf # importing tensorflow takes a long time, so its done after start_time



test_file_name = "train_data_contextMSeconds1000_downsamplingStepSize15_augmentationStepSize20_test.parquet"




model, X, Y = get_model_and_dataset(Path("data")/test_file_name)
load_latest_checkpoint_to_model(model)

Y_actual = np.array([np.argmax(y) for y in Y])
Y_prediction = np.array([np.argmax(y) for y in model.predict(X)])


acc = accuracy_score(Y_actual, Y_prediction)



print(f"Test accuracy: {100*acc:.2f}%")

confusion_matrix = confusion_matrix(Y_actual, Y_prediction)

with open("convert_exercises.json", "r") as f:
    indices_to_exercise_string = json.load(f)["index_to_exercise_string"]
labels = indices_to_exercise_string.values()


plt.style.use("ggplot")

ConfusionMatrixDisplay(confusion_matrix, display_labels=labels).plot()
print(f"Testing took {time.time() - start_time} seconds")


plt.grid(False)
plt.tight_layout()
plt.show()