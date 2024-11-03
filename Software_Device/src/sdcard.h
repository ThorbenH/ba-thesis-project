#pragma once
#include "FS.h"
#include <SD.h>
#include <SPI.h>
#include "pins.h"
#include <Arduino.h>


String  setting_user_name = "UserName";
int     setting_device_number = -1;
char    setting_current_page = 'b';
float   setting_weight_bench = 60;
float   setting_weight_bench_stepsize = 0.5;
float   setting_weight_overhead = 40;
float   setting_weight_overhead_stepsize = 0.5;
float   setting_weight_curl = 25;
float   setting_weight_curl_stepsize = 0.5;
float   setting_weight_squat = 60;
float   setting_weight_squat_stepsize = 0.5;

int setting_bno_accel_offset_x = 0;
int setting_bno_accel_offset_y = 0;
int setting_bno_accel_offset_z = 0;

int setting_bno_mag_offset_x = 0;
int setting_bno_mag_offset_y = 0;
int setting_bno_mag_offset_z = 0;

int setting_bno_gyro_offset_x = 0;
int setting_bno_gyro_offset_y = 0;
int setting_bno_gyro_offset_z = 0;

int setting_bno_accel_radius = 0;
int setting_bno_mag_radius = 0;




String dataLogFilePath = "";

String readLine(File file){
    String line = "";
    char c;
    while(file.available()){
      c = file.read();
      if (c == '\r'){continue;}
      if (c == '\n'){break;}
      line += c;
    }    
    return line;
}

char readLineChar(File file){
    char c;
    char clear;
    while(file.available()){
      c = file.read();
      if (c == 'b' || c == 'o' || c == 'c' || c == 's' || c == 'u'){break;}
    }
    
    while(file.available()){
      clear = file.read();
      if (clear == '\n'){break;}
    }
    return c;
}

void printSettingsToSerial(){
    Serial.println("------------------------------------------");
    Serial.print("setting_user_name ");
    Serial.println(setting_user_name);
    Serial.print("setting_device_number ");
    Serial.println(setting_device_number);
    Serial.print("setting_current_page ");
    Serial.println(setting_current_page);
    Serial.print("setting_weight_bench ");
    Serial.println(setting_weight_bench);
    Serial.print("setting_weight_bench_stepsize ");
    Serial.println(setting_weight_bench_stepsize);
    Serial.print("setting_weight_overhead ");
    Serial.println(setting_weight_overhead);
    Serial.print("setting_weight_overhead_stepsize ");
    Serial.println(setting_weight_overhead_stepsize);
    Serial.print("setting_weight_curl ");
    Serial.println(setting_weight_curl);
    Serial.print("setting_weight_curl_stepsize ");
    Serial.println(setting_weight_curl_stepsize);
    Serial.print("setting_weight_squat ");
    Serial.println(setting_weight_squat);
    Serial.print("setting_weight_squat_stepsize ");
    Serial.println(setting_weight_squat_stepsize);

    Serial.print("setting_bno_accel_offset_x ");
    Serial.println(setting_bno_accel_offset_x);
    Serial.print("setting_bno_accel_offset_y ");
    Serial.println(setting_bno_accel_offset_y);
    Serial.print("setting_bno_accel_offset_z ");
    Serial.println(setting_bno_accel_offset_z);

    Serial.print("setting_bno_mag_offset_x ");
    Serial.println(setting_bno_mag_offset_x);
    Serial.print("setting_bno_mag_offset_y ");
    Serial.println(setting_bno_mag_offset_y);
    Serial.print("setting_bno_mag_offset_z ");
    Serial.println(setting_bno_mag_offset_z);

    Serial.print("setting_bno_gyro_offset_x ");
    Serial.println(setting_bno_gyro_offset_x);
    Serial.print("setting_bno_gyro_offset_y ");
    Serial.println(setting_bno_gyro_offset_y);
    Serial.print("setting_bno_gyro_offset_z ");
    Serial.println(setting_bno_gyro_offset_z);

    Serial.print("setting_bno_accel_radius ");
    Serial.println(setting_bno_accel_radius);
    Serial.print("setting_bno_mag_radius ");
    Serial.println(setting_bno_mag_radius);
    Serial.println("------------------------------------------");
}

bool readSettingsFromSDCard(){
    File file = SD.open("/device_settings.txt");
    if(!file){
        return false;
    }
    setting_user_name = readLine(file);
    setting_device_number = readLine(file).toInt();
    setting_current_page = readLineChar(file);
    setting_weight_bench = readLine(file).toFloat();
    setting_weight_bench_stepsize = readLine(file).toFloat();
    setting_weight_overhead = readLine(file).toFloat();
    setting_weight_overhead_stepsize = readLine(file).toFloat();
    setting_weight_curl = readLine(file).toFloat();
    setting_weight_curl_stepsize = readLine(file).toFloat();
    setting_weight_squat = readLine(file).toFloat();
    setting_weight_squat_stepsize = readLine(file).toFloat();

    
    setting_bno_accel_offset_x = readLine(file).toInt();
    setting_bno_accel_offset_y = readLine(file).toInt();
    setting_bno_accel_offset_z = readLine(file).toInt();

    setting_bno_mag_offset_x = readLine(file).toInt();
    setting_bno_mag_offset_y = readLine(file).toInt();
    setting_bno_mag_offset_z = readLine(file).toInt();

    setting_bno_gyro_offset_x = readLine(file).toInt();
    setting_bno_gyro_offset_y = readLine(file).toInt();
    setting_bno_gyro_offset_z = readLine(file).toInt();

    setting_bno_accel_radius = readLine(file).toInt();
    setting_bno_mag_radius = readLine(file).toInt();

    file.close();
    return true;
}

bool writeSettingsToSDCard(){
    File file = SD.open("/device_settings.txt", FILE_WRITE);
    if(!file){
        return false;
    }

    file.println(setting_user_name);
    file.println(setting_device_number);
    file.println(String(setting_current_page));
    file.println(setting_weight_bench);
    file.println(setting_weight_bench_stepsize);
    file.println(setting_weight_overhead);
    file.println(setting_weight_overhead_stepsize);
    file.println(setting_weight_curl);
    file.println(setting_weight_curl_stepsize);
    file.println(setting_weight_squat);
    file.println(setting_weight_squat_stepsize);

    
    file.println(setting_bno_accel_offset_x);
    file.println(setting_bno_accel_offset_y);
    file.println(setting_bno_accel_offset_z);

    file.println(setting_bno_mag_offset_x);
    file.println(setting_bno_mag_offset_y);
    file.println(setting_bno_mag_offset_z);

    file.println(setting_bno_gyro_offset_x);
    file.println(setting_bno_gyro_offset_y);
    file.println(setting_bno_gyro_offset_z);

    file.println(setting_bno_accel_radius);
    file.println(setting_bno_mag_radius);

    file.close();
    return true;
}

int getNumberOfFiles() {
    File dir = SD.open("/");
    int val = 0;
    while(true) {
        File entry =  dir.openNextFile();
        if (! entry) {
            break;// no more files
        }
        val++;
        entry.close();
    }
    return val;
}

int createNewDataLogFilePath(){//returns file number
    int num = getNumberOfFiles();
    String path = "/device";
    path += setting_device_number;
    #if defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_MAGNETOMETER) && !defined(CALIBRATE_GYROSCOPE)
    path += "_cal_accelerometer.txt";
    #elif defined(CALIBRATE_MAGNETOMETER) && !defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_GYROSCOPE)
    path += "_cal_magnetometer.txt";
    #elif defined(CALIBRATE_GYROSCOPE) && !defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_MAGNETOMETER)
    path += "_cal_gyroscope.txt";
    #else
    path += "_log";
    path += num-1;
    path += ".csv";
    #endif
    dataLogFilePath = path;
    return num;
}

bool createNewDataLogFile(){
    if(dataLogFilePath.isEmpty()){
        return false;
    }

    File myFile = SD.open(dataLogFilePath, FILE_WRITE);
    if (myFile) {
        #if !defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_MAGNETOMETER) && !defined(CALIBRATE_GYROSCOPE)
        myFile.println("barPresent,handPresent,batteryVoltage,currentPage,currentWeight,linAccX,linAccY,linAccZ,AbsQuatW,AbsQuatX,AbsQuatY,AbsQuatZ,accX,accY,accZ,rotX,rotY,rotZ,temp,magX,magY,magZ,absHeight,deltaT");
        #endif
        myFile.close();
        return true;
    }
    return false;
}

bool logData(String data){
    if(dataLogFilePath.isEmpty()){
        return false;
    }

    File myFile = SD.open(dataLogFilePath, FILE_APPEND);
    if (myFile) {
        myFile.print(data);
        myFile.close();
        return true;
    }
    return false;
}

bool setupSDCard() {    
    if(!SD.begin(VSPI_CS_SD_CARD, SPI, 40'000'000)){//increase spi freq to 40 MHz
        return false;
    }
    //printSettingsToSerial();
    if(!readSettingsFromSDCard()){
        return false;
    }
    //printSettingsToSerial();
    return true;
}

