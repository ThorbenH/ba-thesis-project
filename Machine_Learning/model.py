from pathlib import Path
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Input

import pandas as pd
import tensorflow as tf

num_classes = 4

def create_model(input_size, output_size):
    model = Sequential()
    model.add(Input(shape=(input_size,)))
    model.add(Dense(input_size, activation="relu", name="layer_1"))
    model.add(Dense(int(input_size*0.7), activation="relu", name="layer_2"))
    model.add(Dense(output_size, activation="softmax", name="output"))
    return model

def get_model_and_dataset(dataset_path):
    df = pd.read_parquet(dataset_path)
    Y = tf.keras.utils.to_categorical(df["exercise_type"], num_classes=num_classes)
    X = df.drop(columns=["exercise_type"]).to_numpy()
    model = create_model(X.shape[1], num_classes)
    model.compile(optimizer="adam",
                  loss="categorical_crossentropy",
                  metrics=["accuracy"])
    model.summary()
    return model, X, Y

def load_latest_checkpoint_to_model(model):
    latest_checkpoint_path = sorted(Path("checkpoints").glob("*.h5"))[-1]
    print(f"Loading checkpoint {latest_checkpoint_path}")
    model.load_weights(latest_checkpoint_path)