#pragma once
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>


Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);

void displaySensorDetails()
{
    sensor_t sensor;
    bno.getSensor(&sensor);
    Serial.println("------------------------------------");
    Serial.print  ("Sensor:       "); Serial.println(sensor.name);
    Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
    Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
    Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" xxx");
    Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" xxx");
    Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" xxx");
    Serial.println("------------------------------------");
    Serial.println("");
    delay(500);
}

bool setupBNO055()
{
    Serial.begin(115200);
    Serial.println("Orientation Sensor Test"); Serial.println("");

    if(!bno.begin(OPERATION_MODE_IMUPLUS)){
        return false;
    }
    delay(10);
    bno.setExtCrystalUse(false);
    
    return true;
}

bool readBNO055OffsetsToSettings(){
    adafruit_bno055_offsets_t offsets;

    if (!bno.getSensorOffsets(offsets)) {
        return false;
    }
    setting_bno_accel_offset_x = offsets.accel_offset_x;
    setting_bno_accel_offset_y = offsets.accel_offset_y;
    setting_bno_accel_offset_z = offsets.accel_offset_z;

    setting_bno_mag_offset_x = offsets.mag_offset_x;
    setting_bno_mag_offset_y = offsets.mag_offset_y;
    setting_bno_mag_offset_z = offsets.mag_offset_z;

    setting_bno_gyro_offset_x = offsets.gyro_offset_x;
    setting_bno_gyro_offset_y = offsets.gyro_offset_y;
    setting_bno_gyro_offset_z = offsets.gyro_offset_z;

    setting_bno_accel_radius = offsets.accel_radius;
    setting_bno_mag_radius = offsets.mag_radius;
    return true;
}

void writeBNO055OffsetsFromSettings(){
    adafruit_bno055_offsets_t offsets;

    offsets.accel_offset_x = setting_bno_accel_offset_x;
    offsets.accel_offset_y = setting_bno_accel_offset_y;
    offsets.accel_offset_z = setting_bno_accel_offset_z;

    offsets.mag_offset_x = setting_bno_mag_offset_x;
    offsets.mag_offset_y = setting_bno_mag_offset_y;
    offsets.mag_offset_z = setting_bno_mag_offset_z;

    offsets.gyro_offset_x = setting_bno_gyro_offset_x;
    offsets.gyro_offset_y = setting_bno_gyro_offset_y;
    offsets.gyro_offset_z = setting_bno_gyro_offset_z;

    offsets.accel_radius = setting_bno_accel_radius;
    offsets.mag_radius = setting_bno_mag_radius;

    bno.setSensorOffsets(offsets);
}

bool bno055HasCalibrationSettingsStored(){
    if(
        setting_bno_accel_offset_x == 0 &&
        setting_bno_accel_offset_y == 0 &&
        setting_bno_accel_offset_z == 0 &&

        setting_bno_mag_offset_x == 0 &&
        setting_bno_mag_offset_y == 0 &&
        setting_bno_mag_offset_z == 0 &&

        setting_bno_gyro_offset_x == 0 &&
        setting_bno_gyro_offset_y == 0 &&
        setting_bno_gyro_offset_z == 0 &&

        setting_bno_accel_radius == 0 &&
        setting_bno_mag_radius == 0){
        return false;
    }
    return true;
}


void bno055GetCalibration(uint8_t *sys, uint8_t *gyro, uint8_t *accel, uint8_t *mag){
    bno.getCalibration(sys, gyro, accel, mag);
}

bool bno055IsFullyCalibrate(){
    return bno.isFullyCalibrated();
}

void getDataBNO055(double *linAccX, double *linAccY, double *linAccZ, double *quatW, double *quatX, double *quatY, double *quatZ){
    imu::Vector<3> vect = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    imu::Quaternion quat = bno.getQuat();

    *linAccX = vect.x();
    *linAccY = vect.y();
    *linAccZ = vect.z();
    *quatW = quat.w();
    *quatX = quat.x();
    *quatY = quat.y();
    *quatZ = quat.z();
}




void testBNO055()
{
    int64_t beforemesuretime = esp_timer_get_time();
    /*sensors_event_t event;
    bno.getEvent(&event);

    Serial.print(F("Orientation: "));
    Serial.print((float)event.orientation.x);
    Serial.print(F(" "));
    Serial.print((float)event.orientation.y);
    Serial.print(F(" "));
    Serial.print((float)event.orientation.z);
    Serial.println(F(""));
    */
    imu::Quaternion thequat = bno.getQuat();

    imu::Vector<3> vectordata = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
    int64_t timetaken = esp_timer_get_time()-beforemesuretime;
    /*Serial.print(F("Orientation: "));
    Serial.print(thequat.w());
    Serial.print(F(" "));
    Serial.print(thequat.x());
    Serial.print(F(" "));
    Serial.print(thequat.y());
    Serial.print(F(" "));
    Serial.print(thequat.z());
    Serial.println(F(""));
    Serial.print(F("Gravity: "));
    Serial.print(vectordata.x());
    Serial.print(F(" "));
    Serial.print(vectordata.y());
    Serial.print(F(" "));
    Serial.print(vectordata.z());
    Serial.println(F(""));*/


    //Serial.print("m Mesurement took: ");
    Serial.println(timetaken);
    //Serial.println("us");

    uint8_t sys, gyro, accel, mag = 0;
    //bno.getCalibration(&sys, &gyro, &accel, &mag);
    /*Serial.print(F("Calibration: "));
    Serial.print(sys, DEC);
    Serial.print(F(" "));
    Serial.print(gyro, DEC);
    Serial.print(F(" "));
    Serial.print(accel, DEC);
    Serial.print(F(" "));
    Serial.println(mag, DEC);*/

    delay(10);
}
