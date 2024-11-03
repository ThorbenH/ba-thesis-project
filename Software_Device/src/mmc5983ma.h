#pragma once
#include <Wire.h>
#include "pins.h"

#include <SparkFun_MMC5983MA_Arduino_Library.h>

SFE_MMC5983MA myMag;


uint32_t rawValueX = 0;
uint32_t rawValueY = 0;
uint32_t rawValueZ = 0;
double scaledX = 0;
double scaledY = 0;
double scaledZ = 0;



bool setupMMC5983MA()
{
    Wire.begin();

    pinMode(MAG_INT, INPUT);

     if (myMag.begin() == false){
        return false;
    }

    myMag.softReset();

    Serial.println("MMC5983MA connected");

    myMag.setFilterBandwidth(800);
    myMag.setContinuousModeFrequency(1000);
    myMag.enableAutomaticSetReset();
    myMag.enableContinuousMode();
    return true;
}

void getDataMMC5983MA(double *magX, double *magY, double *magZ){
    myMag.readFieldsXYZ(&rawValueX, &rawValueY, &rawValueZ);
    scaledX = (double)rawValueX - 131072.0;
    scaledX /= 131072.0;
    scaledY = (double)rawValueY - 131072.0;
    scaledY /= 131072.0;
    scaledZ = (double)rawValueZ - 131072.0;
    scaledZ /= 131072.0;
    
    scaledX *= 8*100; //full scale to uT (+-800uT)
    scaledY *= 8*100; //full scale to uT (+-800uT)
    scaledZ *= 8*100; //full scale to uT (+-800uT)

    *magX = scaledX;
    *magY = scaledY;
    *magZ = scaledZ;
}

void testMMC5983MA()
{
    
    int64_t beforemesuretime = esp_timer_get_time();
    myMag.readFieldsXYZ(&rawValueX, &rawValueY, &rawValueZ);
    int64_t timetaken = esp_timer_get_time()-beforemesuretime;
    Serial.print("m Mesurement took: ");
    Serial.print(timetaken);
    Serial.println("us");
    Serial.println();

    // The magnetic field values are 18-bit unsigned. The _approximate_ zero (mid) point is 2^17 (131072).
    // Here we scale each field to +/- 1.0 to make it easier to calculate an approximate heading.
    //
    // Please note: to properly correct and calibrate the X, Y and Z channels, you need to determine true
    // offsets (zero points) and scale factors (gains) for all three channels. Futher details can be found at:
    // https://thecavepearlproject.org/2015/05/22/calibrating-any-compass-or-accelerometer-for-arduino/


    scaledX = (double)rawValueX - 131072.0;
    scaledX /= 131072.0;
    scaledY = (double)rawValueY - 131072.0;
    scaledY /= 131072.0;
    scaledZ = (double)rawValueZ - 131072.0;
    scaledZ /= 131072.0;

    // The magnetometer full scale is +/- 8 Gauss
    // Multiply the scaled values by 8 to convert to Gauss
    Serial.print("X axis field (Gauss): ");
    Serial.print(scaledX * 8, 5); // Print with 5 decimal places

    Serial.print("\tY axis field (Gauss): ");
    Serial.print(scaledY * 8, 5);

    Serial.print("\tZ axis field (Gauss): ");
    Serial.println(scaledZ * 8, 5);

    scaledX = (double)rawValueX - 131072.0;
    scaledX /= 131072.0;

    scaledY = (double)rawValueY - 131072.0;
    scaledY /= 131072.0;

    scaledZ = (double)rawValueZ - 131072.0;
    scaledZ /= 131072.0;

    delay(10);
}