#pragma once
#include <Arduino.h>
#include "piezo.h"
#include "pins.h"

volatile int button_center_presses = 0;
volatile int button_down_presses = 0;
volatile int button_right_presses = 0;
volatile int button_up_presses = 0;
volatile int button_left_presses = 0;

volatile int encoder = 0;

volatile int64_t debounceStart_us_CENTER = esp_timer_get_time();
volatile int64_t debounceStart_us_DOWN = esp_timer_get_time();
volatile int64_t debounceStart_us_RIGHT = esp_timer_get_time();
volatile int64_t debounceStart_us_UP = esp_timer_get_time();
volatile int64_t debounceStart_us_LEFT = esp_timer_get_time();
const int buttonDebounceTime_us = 200'000;


#if defined(CALIBRATE_ACCELEROMETER)
volatile int64_t lastPressTimeCenterButton_us = UINT64_MAX/2;//ensure inital value does not trigger calibration
const uint64_t buttonDelayTillCalibrationMeasurement_us = 1'500'000;//1.5sec
const uint64_t buttonCalibrationMeasurementTime_us = 200'000;//0.2sec
#elif defined(CALIBRATE_GYROSCOPE)
volatile int64_t lastPressTimeCenterButton_us = UINT64_MAX/2;//ensure inital value does not trigger calibration
const uint64_t buttonDelayTillCalibrationMeasurement_us = 30'000'000;//30sec
const uint64_t buttonCalibrationMeasurementTime_us = 120'000'000;//2min
#endif


void interruptButton_CENTER(){
    int64_t now = esp_timer_get_time();
    if(debounceStart_us_CENTER + buttonDebounceTime_us > now) {return;}
    debounceStart_us_CENTER = now;
    button_center_presses++;
    setPiezo(250,50);

    #if defined(CALIBRATE_ACCELEROMETER) || defined(CALIBRATE_GYROSCOPE)
    lastPressTimeCenterButton_us = esp_timer_get_time();
    #endif
}

#if defined(CALIBRATE_ACCELEROMETER) || defined(CALIBRATE_GYROSCOPE)
bool takeCalibrationMeasurement(){
    int64_t now = esp_timer_get_time();
    if(lastPressTimeCenterButton_us + buttonDelayTillCalibrationMeasurement_us > now) {return false;}
    if(lastPressTimeCenterButton_us + buttonDelayTillCalibrationMeasurement_us + buttonCalibrationMeasurementTime_us < now) {return false;}
    return true;
}
#endif


void interruptButton_DOWN(){
    int64_t now = esp_timer_get_time();
    if(debounceStart_us_DOWN + buttonDebounceTime_us > now) {return;}
    debounceStart_us_DOWN = now;
    button_down_presses++;
    //Serial.println("Down");
    setPiezo(250,50);
}
void interruptButton_RIGHT(){
    int64_t now = esp_timer_get_time();
    if(debounceStart_us_RIGHT + buttonDebounceTime_us > now) {return;}
    debounceStart_us_RIGHT = now;
    button_right_presses++;
    //Serial.println("Right");
    setPiezo(250,50);
}
void interruptButton_UP(){
    int64_t now = esp_timer_get_time();
    if(debounceStart_us_UP + buttonDebounceTime_us > now) {return;}
    debounceStart_us_UP = now;
    button_up_presses++;
    //Serial.println("Up");
    setPiezo(250,50);
}
void interruptButton_LEFT(){
    int64_t now = esp_timer_get_time();
    if(debounceStart_us_LEFT + buttonDebounceTime_us > now) {return;}
    debounceStart_us_LEFT = now;
    button_left_presses++;
    //Serial.println("Left");
    setPiezo(250,50);
}

void interruptEncoder_A(){
    int a = digitalRead(ENCODER_A);
    int b = digitalRead(ENCODER_B);
    if(a == HIGH && b == LOW || a == LOW && b == HIGH){
        encoder++;
    } else{//if(a == HIGH && b == HIGH || a == LOW && b == LOW)
        encoder--;
    }
    setPiezo(500,5);
}
void interruptEncoder_B(){
    int a = digitalRead(ENCODER_A);
    int b = digitalRead(ENCODER_B);
    if(a == HIGH && b == HIGH || a == LOW && b == LOW){
        encoder++;
    } else {//if(a == HIGH && b == LOW || a == LOW && b == HIGH)
        encoder--;
    }
    setPiezo(500,5);
}

void serialPrintEncoderTest(){
    Serial.print(encoder);
    Serial.println(" Encoder Value");
}

void setupInputWheel(){
    pinMode(BUTTON_CENTER_S1, INPUT);
    pinMode(BUTTON_DOWN_S2, INPUT);
    pinMode(BUTTON_RIGHT_S3, INPUT);
    pinMode(BUTTON_UP_S4, INPUT);
    pinMode(BUTTON_LEFT_S5, INPUT);
    pinMode(ENCODER_A, INPUT);
    pinMode(ENCODER_B, INPUT);
}

void attachInputWheelInterrupts(){ //call from core you want the interrupts to work on
    attachInterrupt(BUTTON_CENTER_S1, interruptButton_CENTER, FALLING);
    attachInterrupt(BUTTON_DOWN_S2, interruptButton_DOWN, FALLING);
    attachInterrupt(BUTTON_RIGHT_S3, interruptButton_RIGHT, FALLING);
    attachInterrupt(BUTTON_UP_S4, interruptButton_UP, FALLING);
    attachInterrupt(BUTTON_LEFT_S5, interruptButton_LEFT, FALLING);
    attachInterrupt(ENCODER_A, interruptEncoder_A, CHANGE);
    attachInterrupt(ENCODER_B, interruptEncoder_B, CHANGE);
}