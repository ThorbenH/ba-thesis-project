#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "pins.h"

#include "images\screenBackground2.h"

#include "fonts\Open_Sans_Condensed_Bold_26.h"
#include "fonts\Open_Sans_Regular_26.h"
#include "fonts\Open_Sans_Regular_35.h"
#include "fonts\Open_Sans_Regular_40.h"
#include "fonts\Open_Sans_Bold_26.h"
#include "fonts\Open_Sans_Bold_35.h"
#include "fonts\Open_Sans_Bold_40.h"

#include "images\buttonRightSmallV2.h"
#include "images\buttonWrongSmallV2.h"

#include "images\bat0.h"
#include "images\bat1.h"
#include "images\bat2.h"
#include "images\bat3.h"
#include "images\bat4.h"
#include "images\bat5.h"

#define TRANSPARENT_BACKGROUND_COLOR_ICONS RGBtoColor(0,0,255)

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite batSprite = TFT_eSprite(&tft);
//TFT_eSprite iconsBig = TFT_eSprite(&tft);
TFT_eSprite iconsSmallSprite = TFT_eSprite(&tft);
TFT_eSprite weightSprite = TFT_eSprite(&tft);
TFT_eSprite weightTextSprite = TFT_eSprite(&tft);

bool recordingIndicatorToggle = false;

void *lastBatImage = (void*)&bat0;
bool lastBarState = false;
bool lastHandState = false;
float lastWeight = 0;


enum Page {
  PAGE_BENCH = 'b',
  PAGE_OVERHEAD = 'o',
  PAGE_CURL = 'c',
  PAGE_SQUAT = 's',
  PAGE_USER = 'u',
};
Page charToPage(char c) {
    return static_cast<Page>(c);
}
char pageToChar(Page page) {
    return static_cast<char>(page);
}


uint16_t RGBtoColor(uint8_t r, uint8_t g, uint8_t b){//turn 8:8:8 into 5:6:5
    uint8_t red = r >> 3;
    uint8_t green = g >> 2;
    uint8_t blue = b >> 3;
    return (red << 11) | (green << 5) | blue;
}

void drawStatusButton(bool status, int x, int y){
    if(status){
        iconsSmallSprite.pushImage(0,0,48,48, buttonRightSmallV2);
    } else{
        iconsSmallSprite.pushImage(0,0,48,48, buttonWrongSmallV2);
    }
    iconsSmallSprite.pushSprite(x-24,y-24,TRANSPARENT_BACKGROUND_COLOR_ICONS);
}

void displayBatteryPercentage(float voltage, bool forceDisplay = false){
    //batSprite.setSwapBytes(true); // somehow pushrotated performs this, so we should not call this, maybe related https://github.com/Bodmer/TFT_eSPI/issues/1359
    void *newBatImage;
    if(voltage < 3.6){
        newBatImage = (void*)&bat0;
    } else if(voltage < 3.7){
        newBatImage = (void*)&bat1;
    } else if(voltage < 3.75){
        newBatImage = (void*)&bat2;
    } else if(voltage < 3.8){
        newBatImage = (void*)&bat3;
    } else if(voltage < 3.85){
        newBatImage = (void*)&bat4;
    } else {
        newBatImage = (void*)&bat5;
    }
    if(lastBatImage != newBatImage || forceDisplay){//only update if necessary
        lastBatImage = newBatImage;
        batSprite.pushImage(0,0,48,48, (const uint16_t *)lastBatImage);
        tft.setPivot(208, 32);
        batSprite.pushRotated(90,TFT_BLACK);
    }

}
void displayDeviceNumber(int number){
    tft.setTextColor(TFT_WHITE);
    String text = "dev"+String(number);
    tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
    tft.drawString(text, 120, 20);
}
void displayName(String name){
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
    tft.drawString(name, 15, 20);
}

void displayWeight(float weight, bool forceDisplay = false){
    if(lastWeight != weight || forceDisplay){
        lastWeight = weight;
        tft.setTextColor(TFT_WHITE);
        String text = String(weight)+"KG";
        weightSprite.pushImage(-10,-170,240,280, screenBackground2);
        weightTextSprite.fillSprite(TFT_BLACK);
        weightTextSprite.setFreeFont(&Open_Sans_Bold_35);
        weightTextSprite.drawString(text, 0, 0);
        weightTextSprite.pushToSprite(&weightSprite, 0,0,TFT_BLACK);
        weightSprite.pushSprite(10,170);
    }
}

void displayBarState(bool state, bool forceDisplay = false){
    if(forceDisplay){
        lastBarState = state;
        tft.setTextColor(TFT_WHITE);
        tft.setFreeFont(&Open_Sans_Bold_26);
        tft.drawString("Bar", 185, 222-50-20);
        drawStatusButton(lastBarState, 240-24-10, 222-20);
    } else if(lastBarState != state){
        lastBarState = state;
        drawStatusButton(lastBarState, 240-24-10, 222-20);
    }
}

void displayHandState(bool state, bool forceDisplay = false){
    if(forceDisplay){
        lastHandState = state;
        tft.setTextColor(TFT_WHITE);
        tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
        tft.drawString("Hand", 178, 222-130-20);
        drawStatusButton(lastHandState, 240-24-10, 222-80-20);
    } else if(lastHandState != state){
        lastHandState = state;
        drawStatusButton(lastHandState, 240-24-10, 222-80-20);
    }
}

void displayDataLogFilePath(){
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
    tft.drawString(dataLogFilePath, 20, 249);
}


void drawBorder(int border_width){
    int display_radius = 43;
    int line_width = border_width*2-2;
    uint16_t color = TFT_GREEN;
    uint16_t backgroundcolor = TFT_GREEN;
    tft.drawWideLine(0, 0, 0, tft.height(), line_width+2,color);
    tft.drawWideLine(0, 0, tft.width(), 0, line_width+2,color);
    tft.drawWideLine(tft.width(), tft.height(), 0, tft.height(), line_width+2,color);
    tft.drawWideLine(tft.width(), tft.height(), tft.width(), 0, line_width+2,color);
    tft.drawArc(0+display_radius, tft.height()-display_radius, display_radius, display_radius-line_width/2, 0, 90, color, backgroundcolor, true);
    tft.drawArc(tft.width()-display_radius, tft.height()-display_radius, display_radius, display_radius-line_width/2, 270, 0, color, backgroundcolor, true);
    tft.drawArc(tft.width()-display_radius, display_radius, display_radius, display_radius-line_width/2, 180, 270, color, backgroundcolor, true);
    tft.drawArc(display_radius, display_radius, display_radius, display_radius-line_width/2, 90, 180, color, backgroundcolor, true);
}

void setupDisplay() {
    //pinMode(DISPLAY_BL, OUTPUT); //if you dont define DTFT_BACKLIGHT_ON this is an alternative to getting the backlight to light up
    //digitalWrite(DISPLAY_BL, HIGH);

    tft.init();

    tft.setSwapBytes(true);
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    batSprite.createSprite(48,48);
    iconsSmallSprite.createSprite(48,48);
    iconsSmallSprite.setSwapBytes(true);

    weightTextSprite.createSprite(175, 35);
    weightSprite.createSprite(175, 35);
    weightSprite.setSwapBytes(true);

    /*iconsBig.createSprite(72,72);
    iconsBig.setSwapBytes(true);
    iconsBig.pushImage(0,0,72,72, circle_grey_big_b);
    iconsBig.pushSprite(100,100,TRANSPARENT_BACKGROUND_COLOR_ICONS);*/
}


void displayStartupScreen(){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Bold_35);
    delay(50);
    tft.drawString("Initializing...", 10, 140);
    delay(250);

    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    delay(50);
    tft.drawString("SD Card:", 10, 20);
    delay(50);
    tft.drawString("IMU:", 10, 70);
    delay(50);
    tft.drawString("MAG:", 10, 120);
    delay(50);
    tft.drawString("BARO:", 10, 170);
    delay(50);
    tft.drawString("IMU-2:", 10, 220);
}




void displayStartupState(int devStartNumber, bool state){
    drawStatusButton(state, 200, 35+50*devStartNumber);
}

void displayFailPage(String message){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    tft.setTextColor(TFT_RED);
    tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
    delay(250);
    tft.drawString(message, 10, 40);
    delay(50);
    tft.drawString("Please try restarting", 10, 100);
    delay(50);
    tft.drawString("Contact support", 10, 160);
    delay(50);
    tft.drawString("if problem persists.", 10, 190);
    delay(50);
}


void displayBNO055CalibrationScreen(uint8_t sys, uint8_t gyro, uint8_t accel, uint8_t mag){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    tft.setTextColor(TFT_YELLOW);
    tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
    tft.drawString("BNO055 calibration:", 10, 30);
    tft.drawString("Please perform pro-", 10, 60);
    tft.drawString("cedure, if not adequate.", 10, 90);
    
    String text = "System: "+String(sys);
    tft.drawString(text, 10, 150);
    text = "Gyro: "+String(gyro);
    tft.drawString(text, 10, 180);
    text = "Accel: "+String(accel);
    tft.drawString(text, 10, 210);
    text = "Mag: "+String(mag);
    tft.drawString(text, 10, 240);
}


void displayBNO055CalibratedScreen(){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Bold_35);
    delay(50);
    tft.drawString("BNO055", 10, 100);
    delay(50);
    tft.drawString("Calibrated!", 10, 140);
}

void displayStartingRecording(){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Bold_35);
    delay(50);
    tft.drawString("Starting", 10, 100);
    delay(50);
    tft.drawString("Recording!", 10, 140);
}

void updatePage(Page page, bool handState, bool barState, float batteryVoltage, float weight, bool forceDisplay = false){
    if(page == PAGE_USER){
        return;
    }
    displayHandState(handState, forceDisplay);
    displayBarState(barState, forceDisplay);
    displayBatteryPercentage(batteryVoltage, forceDisplay);
    displayWeight(weight, forceDisplay);
}

//void drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color){
//    tft.drawLine(ax, ay, bx, by, );
//}

void displayPage(Page page){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Bold_35);
    switch (page)
    {
        case PAGE_BENCH:
            tft.drawString("Bench", 10, 95);
            break;
        case PAGE_OVERHEAD:
            tft.drawString("Over-", 10, 80);
            tft.drawString("head", 10, 110);
            
            break;
        case PAGE_CURL:
            tft.drawString("Curl", 10, 95);
            break;
        case PAGE_SQUAT:
            tft.drawString("Squat", 10, 95);
            break;
        default:
            tft.setTextColor(TFT_WHITE);
            tft.setFreeFont(&Open_Sans_Condensed_Bold_26);
            tft.drawString("Device created for", 10, 10+20);
            tft.drawString("bachelors thesis.", 10, 40+20);
            tft.drawString("Owner: ThorbenHorn", 10, 70+20);
            tft.setTextColor(TFT_YELLOW);
            tft.drawString("\tTel: 0160 651 6654", 10, 100+20);
            tft.setTextColor(TFT_GREENYELLOW);
            tft.drawString("\thorn.thorben@", 10, 130+20);
            tft.drawString("hm.edu", 10, 160+20);            
            tft.setTextColor(TFT_WHITE);
            tft.drawString("Participant:", 10, 190+20);
            tft.drawString(setting_user_name, 10, 220+20);
            return;
    }
    tft.drawWideLine(0, 55, tft.width(), 55, 10, TFT_BLACK, TFT_BLACK);
    tft.drawWideLine(0, 235, tft.width(), 235, 10,TFT_BLACK, TFT_BLACK);
    tft.drawWideLine(180, 0, 180, 50, 5,TFT_BLACK, TFT_BLACK);
    tft.drawWideLine(110, 0, 110, 50, 5,TFT_BLACK, TFT_BLACK);
    displayName(setting_user_name);
    displayDeviceNumber(setting_device_number);
    displayDataLogFilePath();
}

void displayTodo(String message){
    tft.pushImage(0,0,tft.width(), tft.height(), screenBackground2);

    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(&Open_Sans_Bold_35);
    delay(50);
    tft.drawString("TODO", 10, 100);
    tft.drawString(message, 10, 150);
}





void testDisplay() {
    tft.invertDisplay( false );
    tft.fillScreen(TFT_BLACK);
    tft.drawRect(0, 0, tft.width(), tft.height(), TFT_GREEN);
    tft.setCursor(0, 4, 4);
    tft.setTextColor(TFT_WHITE);
    tft.println(" Invert OFF\n");
    tft.println(" White text");
    tft.setTextColor(TFT_RED);
    tft.println(" Red text");
    tft.setTextColor(TFT_GREEN);
    tft.println(" Green text");
    tft.setTextColor(TFT_BLUE);
    tft.println(" Blue text");
    delay(500);
    //tft.invertDisplay( true ); // Binary inversion of colours
}