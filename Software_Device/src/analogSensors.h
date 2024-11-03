#pragma once
#include <Arduino.h>
#include "pins.h"

static const float voltage_divider = 55.1/33;
static const float voltage_offset = 0.2;

int barSensorValue = 4000;
int handSensorValue = 4000;

bool barPresent = false;
bool handPresent = true;
float batteryVoltage = 0;

void setupAnalogSensors(){
    pinMode(ANALOG_BAT_SENS, INPUT);
    pinMode(ANALOG_BAR_SENS, INPUT);
    pinMode(ANALOG_HAND_SENS, INPUT);
}

void updateBatteryVoltage(){
    float value = analogRead(ANALOG_BAT_SENS);
    batteryVoltage = value*3.3*voltage_divider/4095+voltage_offset;
}

int readBarSensor(){
    barSensorValue = barSensorValue*0.8+analogRead(ANALOG_BAR_SENS)*0.2;
    return barSensorValue;
}

int readHandSensor(){
    handSensorValue = handSensorValue*0.8+analogRead(ANALOG_HAND_SENS)*0.2;
    return handSensorValue;
}

void updateBarPresent(){
    readBarSensor();
    barPresent = barSensorValue<500;
}
void updateHandPresent(){
    readHandSensor();
    handPresent = handSensorValue<500;
}

void serialPrintAnalogSensorTest(){
    Serial.print("BatteryVoltage: ");
    updateBatteryVoltage();
    Serial.print(batteryVoltage);
    Serial.print("V");
    Serial.print("\tHandSensor: ");
    Serial.print(readHandSensor());
    Serial.print("\tBarSensor: ");
    Serial.println(readBarSensor());
    delay(20);
}