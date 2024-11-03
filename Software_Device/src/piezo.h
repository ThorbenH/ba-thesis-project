#pragma once
#include <Arduino.h>
#include "pins.h"

void setupPiezo(){
    pinMode(PIEZO, OUTPUT);
    digitalWrite(PIEZO, LOW);
}


void setPiezo(int frequency, int duration){
    tone(PIEZO, frequency, duration);
    //delay(duration);
    //noTone(PIEZO);
}
