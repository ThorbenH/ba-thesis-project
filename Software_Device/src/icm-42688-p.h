#pragma once
#include "ICM42688.h"

ICM42688 IMU(Wire, 0x69);

bool setupICM42688P() {
    pinMode(IMU_INT1, INPUT);
    
    int status = IMU.begin();
	if (status < 0) {
		return false;
	}

	
	IMU.setAccelFS(ICM42688::gpm4);// accelerometer range +/-4G
	IMU.setGyroFS(ICM42688::dps250);// gyroscope range +/-250 deg/s
	IMU.setAccelODR(ICM42688::odr1k);
	IMU.setGyroODR(ICM42688::odr1k);

	return true;

}


void getDataICM42688PA(float *accX, float *accY, float *accZ, float *rotX, float *rotY, float *rotZ, float *temp){
    IMU.getAGT();
    *accX = IMU.accX()*9.81;//g -> m/s^2
    *accY = IMU.accY()*9.81;
    *accZ = IMU.accZ()*9.81;
    *rotX = IMU.gyrX();
    *rotY = IMU.gyrY();
    *rotZ = IMU.gyrZ();
    *temp = IMU.temp();
}

void testICM42688P() {
	// read the sensor
    int64_t beforemesuretime = esp_timer_get_time();
	IMU.getAGT();
    int64_t timetaken = esp_timer_get_time()-beforemesuretime;
    Serial.print("m Mesurement took: ");
    Serial.print(timetaken);
    Serial.println("us");
    Serial.println();

	// display the data
	/*Serial.println("ax,ay,az,gx,gy,gz,temp_C");
	
	Serial.print(IMU.accX(), 6);
	Serial.print("\t");
	Serial.print(IMU.accY(), 6);
	Serial.print("\t");
	Serial.print(IMU.accZ(), 6);
	Serial.print("\t");
	Serial.print(IMU.gyrX(), 6);
	Serial.print("\t");
	Serial.print(IMU.gyrY(), 6);
	Serial.print("\t");
	Serial.print(IMU.gyrZ(), 6);
	Serial.print("\t");
	Serial.println(IMU.temp(), 6);*/
	delay(1);
}