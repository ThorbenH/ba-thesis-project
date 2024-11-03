
import time
from pathlib import Path
from model import get_model_and_dataset, load_latest_checkpoint_to_model
import shutil
import pandas as pd
from matplotlib import pyplot as plt

start_time = time.time()
import tensorflow as tf # importing tensorflow takes a long time, so its done after start_time



training_file_name = "train_data_contextMSeconds1000_downsamplingStepSize15_augmentationStepSize20_train.parquet"



if Path("checkpoints").exists():
    #response = None
    #while response not in ["yes", "no"]:
    #    response = input("Do you want to delete previous checkpoints and start over? (yes/no): ").strip().lower()
    #if response == "yes":
    shutil.rmtree("checkpoints")
    Path("checkpoints").mkdir()

model, X, Y = get_model_and_dataset(Path("data")/training_file_name)

tf.keras.utils.plot_model(# pip install pydot ??????????????? pydotplus graphviz
    model,
    to_file="model_architecture.pdf",
    show_shapes=True,
    show_dtype=True,
    show_layer_names=True,
    rankdir="TB", # TB = vertical, LR = horizontal
    show_layer_activations=True,
    show_trainable=True,
)

checkpoint_formatting = "checkpoints/model_{epoch:03d}-{accuracy:.4f}.weights.h5"
callback_checkpoint = tf.keras.callbacks.ModelCheckpoint(
    filepath=checkpoint_formatting,
    save_weights_only=True
)
callback_earlystop = tf.keras.callbacks.EarlyStopping(
    monitor="val_loss",
    patience=5,
    restore_best_weights=True
)
callback_logger = tf.keras.callbacks.CSVLogger("checkpoints/.log") #track accuracy and loss

model.save_weights(checkpoint_formatting.format(epoch=0, accuracy=0.0))
load_latest_checkpoint_to_model(model)
model.fit(
    X,
    Y,
    epochs=50,
    batch_size=16,
    validation_split=0.1,
    callbacks=[
        callback_checkpoint,
        # callback_earlystop,
        callback_logger,
    ],
)

print(f"Training took {time.time() - start_time} seconds")

# visualize training
df  = pd.read_csv("checkpoints/.log")
plt.style.use("ggplot")
plt.plot(df["epoch"], df["accuracy"], "C0", label="Training Accuracy")
plt.plot(df["epoch"], df["val_accuracy"], "C0--", label="Validation Accuracy")
plt.ylabel("Accuracy")
plt.xlabel("Epoch")
plt.ylim(0, 1)
plt.twinx()
plt.plot([], [], "C0", label="Training Accuracy")#add empty plots for legend
plt.plot([], [], "C0--", label="Validation Accuracy")#add empty plots for legend
plt.plot(df["epoch"], df["loss"], "C1", label="Training Loss")
plt.plot(df["epoch"], df["val_loss"], "C1--", label="Validation Loss")
plt.ylabel("Loss")
plt.xlabel("Epoch")
plt.legend(loc = "lower left")
plt.show()

print("DONE, model saved to checkpoints folder, execute again to continue training")