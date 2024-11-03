# BarbelMotionTracker

This git project is dedicated to a prototype implementation that attempts to track and evaluate the motion of a barbell in the gym.

## 3D-Files

Here you will find the FFF (FDM) printable files for a case for the board. All STL-files are in metric (mm). To complete the assembly you will need some additional hardware (M3 screws, rubber pad, M3 heat set inserts, 4mmx0.5mm brass tubing, 2cm wide velcro strap)

## PCB_Design

Here you will find the KiCad8.0 design files as well as the pre exported gerbers (ready to order at JLCPCB). You will also find the ibom.html file which is a Interactive Bill Of Materials that you can open in any modern browser. It is of great use when doing board smd assembly.

## Software_Device

Here you will find a PlatformIO project that contains the software running on the board. It is written in C/C++ for the esp32 platform using the arduino framework.

To get started install VSCode, install the PlatformIO extension, plug in the board using the USB-C shared power and data port and make sure the board is turned on using the power switch.p

The software collects all the raw sensor data into a CSV-File for future of board processing.

Calibration modes can be selected by uncommenting preprocessor arguments in the "platformio.ini" file.

### Micro SD-Card

The sd-card has to be formatted as FAT32. FAT32 is only supported by micro-sd-cards with capacity less than 32GB.
The sd-card has to contain a settings.txt file that has to be populated. Check the "sdcard.h" code for details.
A good starting point might be this (make sure to include the empty lines at the end of the file, as the device expects these to write the BNO055 calibration to):

```
User
1
s
90.00
2.50
45.00
2.50
12.50
2.50
95.00
2.50

















```

## Software_Device

Contains a the python code to visualize a calibration as gathered by the device (see Software_Device).
To get the calibration matrix you can use the tool Magneto v1.2 at https://sites.google.com/view/sailboatinstruments1 .


## Post_Process_Data

First run "apply_calibration_and_split.py" to split one recording session into exercises and sets and apply the calibration. This requires a recorded session and a "cal_matrix.json" file in the subfolder "data". Make sure to set the correct file name for your recordig in "apply_calibration_and_split.py".
Next you can run "compute_orientation_and_motion.py". Make sure to set the correct target folder.

Some settings are required for these two steps. They can be set in "processing_settings.py".

All other files should contain "_thesis" in their name and where only used to create plots of the data for the thesis. Feel free to disregard these.

## Machine_Learning
To run the machine learning code you first need to create the dataset used for training and testing. The python code file "prepData.py" uses a hard coded location for the data. To generate the data required first run the Post_Process_Data pipeline. Then you can run the training in order "prepData.py" -> "train.py" -> "test.py".