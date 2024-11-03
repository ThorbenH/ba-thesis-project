#pragma once
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"

static const float sealevelpressure_HPA = 1022.0;
static const int i2c_address_bme390 = 0x76;


Adafruit_BMP3XX bmp;

bool setupBMP390() {
    if (!bmp.begin_I2C(i2c_address_bme390)) {
        return false;
    }
    bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_2X);
    bmp.setPressureOversampling(BMP3_OVERSAMPLING_16X);
    bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
    bmp.setOutputDataRate(BMP3_ODR_25_HZ);
    return true;
}

void getDataBMP390(float *alt){
    *alt = bmp.readAltitude(sealevelpressure_HPA);//altitude in meters
}


void testBMP390() {
    //if (! bmp.performReading()) {
    //    Serial.println("Failed to perform reading :(");
    //    return;
    //}
    //Serial.print("Temperature = ");
    //Serial.print(bmp.temperature);
    //Serial.println(" *C");

    //Serial.print("Pressure = ");
    //Serial.print(bmp.pressure / 100.0);
    //Serial.println(" hPa");

    
    int64_t beforemesuretime = esp_timer_get_time();

    Serial.print("Approx. Altitude = ");
    Serial.print(bmp.readAltitude(sealevelpressure_HPA));
    int64_t timetaken = esp_timer_get_time()-beforemesuretime;
    Serial.print("m Mesurement took: ");
    Serial.print(timetaken);
    Serial.println("us");
    delay(100);
}