#include <Arduino.h>
#include "pins.h"

#include "analogSensors.h"
#include "inputWheel.h"
#include "piezo.h"
#include "sdcard.h"
#include "display.h"
#include "bmp390.h"
#include "bno055.h"
#include "mmc5983ma.h"
#include "icm-42688-p.h"

static const int core_sd_card_io = 0; //PRO_CPU | wifi/bt core by default | has special watchdog if scheduler isn't triggered in long time -> resets microcontroler (so wireless is not stuck in loop)
static const int core_sensors    = 1; //APP_CPU | app core by default

static const int read_sensors_hardware_timer = 0; 
static const uint16_t freq_timer_divider = 80;      // Divide timer freq (80 MHz) by this -> new clock rate is 1mhz
static const uint64_t timer_count = 2000;               // Timer counts to this value
// freq_timer_divider and timer_count define sensor reading frequency -> 500Hz
static const int taskHandleIOYieldTime_ms = 5; //5ms are by far enough for the higher priority sd task to take over
static const int taskSDCardFreq = 10; //in Hz | approximate as the io task might hold the spi sem
static const int sensorReadingsQueueLen = 100; //number of elements

static const int delayBetweenSensorReads_us = 1000000/500; // 1m/500Hz
static const int bno055DelayBetweenReads_us = 1000000/100; // 1m/100Hz
static const int bmp390DelayBetweenReads_us = 1000000/25; // 1m/25Hz

static const float dataNotAvailable = -10000;//out of range for sensor reading -> must be only this


static const int encoderScaleDown = 2;


static hw_timer_t *hwTimerReadSensors = NULL;
static SemaphoreHandle_t semTimerReadSensors = NULL;

static SemaphoreHandle_t spiControlMutex;

static QueueHandle_t sensorReadingsQueue;

TaskHandle_t taskHandleReadSensors;
TaskHandle_t taskHandleHandleIO;
TaskHandle_t taskHandleSDCard;

struct Measurement_t {
  double lin_acc_x, lin_acc_y, lin_acc_z;                // BNO055               | linear acceleration, double [meters/second^2]
  double abs_quat_w, abs_quat_x, abs_quat_y, abs_quat_z; // BNO055               | absolute orientation, double [quaternion]
  float acc_x, acc_y, acc_z;                             // ICM 42688P           | acceleration, float  [meters/second^2]
  float rot_x, rot_y, rot_z;                             // ICM 42688P           | angular velocity, float [degrees/second]
  float temp;                                            // ICM 42688P not used  | temperature gyro die, float [Celsius]
  double mag_x, mag_y, mag_z;                            // MCC5983MA            | double [micro Tesla] (+/-800 uT)
  float abs_height;                                      // BMP390               | altitude, float [meters]
  int64_t time_since_last_measure_us;                    //                      | int [micro seconds]
};

void delayNonBlockingMillis(int millis){
    if(millis <=0){
        return;
    }
    vTaskDelay(millis / portTICK_PERIOD_MS);
}

void IRAM_ATTR hwTimerReadSensorsCallback(){
    BaseType_t task_woken = pdFALSE;
    xSemaphoreGiveFromISR(semTimerReadSensors, &task_woken);//useTaskNotify instead?
    if (task_woken) {
        portYIELD_FROM_ISR();
    }
}

int64_t lastMeasurementTime_us = esp_timer_get_time();
int64_t currentMeasurementTime_us = esp_timer_get_time();

int64_t lastMeasurementTimeBNO055_us = esp_timer_get_time();
int64_t lastMeasurementTimeBMP390_us = esp_timer_get_time();

Measurement_t readMesurement;
void taskReadSensors(TimerHandle_t xTimer){
    hwTimerReadSensors = timerBegin(read_sensors_hardware_timer, freq_timer_divider, true);//use esptimer instead?
    timerAttachInterrupt(hwTimerReadSensors, &hwTimerReadSensorsCallback, true);
    timerAlarmWrite(hwTimerReadSensors, timer_count, true);
    timerAlarmEnable(hwTimerReadSensors);

    lastMeasurementTimeBMP390_us = esp_timer_get_time()-bmp390DelayBetweenReads_us+delayBetweenSensorReads_us;//delay BMP390 first read so it does not occur concurrently with BNO055
    while(1) {
        if (xSemaphoreTake(semTimerReadSensors, portMAX_DELAY) == pdFALSE) {
            continue;
        }
        currentMeasurementTime_us = esp_timer_get_time();
        if(lastMeasurementTimeBNO055_us + bno055DelayBetweenReads_us <= currentMeasurementTime_us){//100Hz
            getDataBNO055(&(readMesurement.lin_acc_x), &(readMesurement.lin_acc_y), &(readMesurement.lin_acc_z),
                        &(readMesurement.abs_quat_w), &(readMesurement.abs_quat_x), &(readMesurement.abs_quat_y), &(readMesurement.abs_quat_z));
            lastMeasurementTimeBNO055_us = currentMeasurementTime_us;
        } else {
            readMesurement.lin_acc_x = dataNotAvailable;
            readMesurement.lin_acc_y = dataNotAvailable;
            readMesurement.lin_acc_z = dataNotAvailable;
            readMesurement.abs_quat_w = dataNotAvailable;
            readMesurement.abs_quat_x = dataNotAvailable;
            readMesurement.abs_quat_y = dataNotAvailable;
            readMesurement.abs_quat_z = dataNotAvailable;
        }

        
        getDataICM42688PA(&(readMesurement.acc_x), &(readMesurement.acc_y), &(readMesurement.acc_z),
            &(readMesurement.rot_x), &(readMesurement.rot_y), &(readMesurement.rot_z),
            &(readMesurement.temp));
        getDataMMC5983MA(&(readMesurement.mag_x), &(readMesurement.mag_y), &(readMesurement.mag_z));

        if(lastMeasurementTimeBMP390_us + bmp390DelayBetweenReads_us <= currentMeasurementTime_us){//25Hz
            getDataBMP390(&(readMesurement.abs_height));
            lastMeasurementTimeBMP390_us = currentMeasurementTime_us;
        } else {
            readMesurement.abs_height = dataNotAvailable;
        }

        readMesurement.time_since_last_measure_us = currentMeasurementTime_us-lastMeasurementTime_us;
        if (xQueueSend(sensorReadingsQueue, (void *)&readMesurement, portTICK_PERIOD_MS) == pdFALSE) {
            Serial.println("---------------------------------!!! Lost data !!!---------------------------------");
            continue;//queue was full and timeout from waiting... we have a big issue
        }
        lastMeasurementTime_us = currentMeasurementTime_us;
    }
}


float getWeight(Page page){
    switch (page)
    {
        case PAGE_BENCH:
            return setting_weight_bench;
        case PAGE_OVERHEAD:
            return setting_weight_overhead;
        case PAGE_CURL:
            return setting_weight_curl;
        case PAGE_SQUAT:
            return setting_weight_squat;
        default:
            return dataNotAvailable;
    }
}

float getWeightStepSize(Page page){
    switch (page)
    {
        case PAGE_BENCH:
            return setting_weight_bench_stepsize;
        case PAGE_OVERHEAD:
            return setting_weight_overhead_stepsize;
        case PAGE_CURL:
            return setting_weight_curl_stepsize;
        case PAGE_SQUAT:
            return setting_weight_squat_stepsize;
        default:
            return 0;
    }
}


void setWeight(Page page, float weight){//requires spiwrite -> make sure you have permission
    switch (page)
    {
        case PAGE_BENCH:
            setting_weight_bench = weight;
            break;
        case PAGE_OVERHEAD:
            setting_weight_overhead = weight;
            break;
        case PAGE_CURL:
            setting_weight_curl = weight;
            break;
        case PAGE_SQUAT:
            setting_weight_squat = weight;
            break;
        default:
            break;
    }
    writeSettingsToSDCard();
}

void moveRight(){
    switch (setting_current_page)
    {
        case PAGE_BENCH:
            setting_current_page = pageToChar(PAGE_OVERHEAD);
            break;
        case PAGE_OVERHEAD:
            setting_current_page = pageToChar(PAGE_CURL);
            break;
        case PAGE_CURL:
            setting_current_page = pageToChar(PAGE_SQUAT);
            break;
        case PAGE_SQUAT:
            setting_current_page = pageToChar(PAGE_USER);
            break;
        default:
            setting_current_page = pageToChar(PAGE_BENCH);
            break;
    }
    writeSettingsToSDCard();
}

void moveLeft(){
    switch (setting_current_page)
    {
        case PAGE_BENCH:
            setting_current_page = pageToChar(PAGE_USER);
            break;
        case PAGE_OVERHEAD:
            setting_current_page = pageToChar(PAGE_BENCH);
            break;
        case PAGE_CURL:
            setting_current_page = pageToChar(PAGE_OVERHEAD);
            break;
        case PAGE_SQUAT:
            setting_current_page = pageToChar(PAGE_CURL);
            break;
        default:
            setting_current_page = pageToChar(PAGE_SQUAT);
            break;
    }
    writeSettingsToSDCard();
}

void movePage(int left, int right){
    for(int i = 0; i<left; i++){
        moveLeft();
    }
    for(int i = 0; i<right; i++){
        moveRight();
    }
}

void taskHandleIO(TimerHandle_t xTimer){
    attachInputWheelInterrupts();
    if (xSemaphoreTake(spiControlMutex, portMAX_DELAY) == pdTRUE) {//display inital upon startup
        displayPage(charToPage(setting_current_page));
        updateBarPresent();
        updateHandPresent();
        updatePage(charToPage(setting_current_page), handPresent, barPresent, batteryVoltage, getWeight(charToPage(setting_current_page)), true);
        xSemaphoreGive(spiControlMutex);
    }
    while(1) {
        delayNonBlockingMillis(taskHandleIOYieldTime_ms);
        if (xSemaphoreTake(spiControlMutex, portMAX_DELAY) == pdFALSE) {
            continue;
        }
        updateBarPresent();
        updateHandPresent();
        //Serial.println("Screen update");
        if(button_right_presses-button_left_presses != 0){
            movePage(button_left_presses, button_right_presses);

            button_right_presses = 0;
            button_left_presses = 0;

            displayPage(charToPage(setting_current_page));
            updatePage(charToPage(setting_current_page), handPresent, barPresent, batteryVoltage, getWeight(charToPage(setting_current_page)), true);
        }
        if(abs(encoder) > encoderScaleDown){
            int change = (encoder/encoderScaleDown);
            float newWeight = getWeight(charToPage(setting_current_page)) + change*getWeightStepSize(charToPage(setting_current_page));
            
            encoder -= change*encoderScaleDown;
            
            setWeight(charToPage(setting_current_page), newWeight);
        }
        updatePage(charToPage(setting_current_page), handPresent, barPresent, batteryVoltage, getWeight(charToPage(setting_current_page)), false);
        xSemaphoreGive(spiControlMutex);
    }
}

int taskSDCardYieldCalc_ms(int timeTaken_us){
    int available_us = 1'000'000/taskSDCardFreq;
    int sleep_time_ms = (available_us-timeTaken_us)/1000;
    //Serial.print("Sleeping: ");
    //Serial.println(sleep_time_ms);
    if(sleep_time_ms<0){
        return 0;
    }
    return sleep_time_ms;
}

char doubleToStringBuffer[50];
String doubleToString(double value) {//uses 5 decimal places
    if(value==dataNotAvailable){
        return "x";
    }
    sprintf(doubleToStringBuffer, "%.5lf", value);
    return String(doubleToStringBuffer);
}

char floatToString2DecimalsBuffer[50];
String floatToString2Decimals(float value) {//uses 2 decimal places
    if(value==dataNotAvailable){
        return "x";
    }
    sprintf(floatToString2DecimalsBuffer, "%.2f", value);
    return String(floatToString2DecimalsBuffer);
}

Measurement_t writeMesurement;
void taskSDCard(TimerHandle_t xTimer){
    int64_t taskSDCardStartTime = esp_timer_get_time();
    int64_t stringDoneTime= esp_timer_get_time();
    int64_t taskDoneTime = esp_timer_get_time();
    while(1) {
        delayNonBlockingMillis(taskSDCardYieldCalc_ms(taskDoneTime-taskSDCardStartTime));
        if (xSemaphoreTake(spiControlMutex, portMAX_DELAY) == pdFALSE) {
            continue;
        }
        taskSDCardStartTime = esp_timer_get_time();
        updateBarPresent();
        updateHandPresent();
        updateBatteryVoltage();
        String text = "";
        int freeSpace = uxQueueSpacesAvailable(sensorReadingsQueue);
        while(xQueueReceive(sensorReadingsQueue, (void *)&writeMesurement, 0) == pdTRUE) {
            
            #if defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_MAGNETOMETER) && !defined(CALIBRATE_GYROSCOPE)
            if(takeCalibrationMeasurement()){
                text += doubleToString(writeMesurement.acc_x); text += "\t";
                text += doubleToString(writeMesurement.acc_y); text += "\t";
                text += doubleToString(writeMesurement.acc_z); text += "\n";
            }
            #elif defined(CALIBRATE_MAGNETOMETER) && !defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_GYROSCOPE)
            text += doubleToString(writeMesurement.mag_x); text += "\t";
            text += doubleToString(writeMesurement.mag_y); text += "\t";
            text += doubleToString(writeMesurement.mag_z); text += "\n";
            #elif defined(CALIBRATE_GYROSCOPE) && !defined(CALIBRATE_ACCELEROMETER) && !defined(CALIBRATE_MAGNETOMETER)
            if(takeCalibrationMeasurement()){
                text += doubleToString(writeMesurement.rot_x); text += "\t";
                text += doubleToString(writeMesurement.rot_y); text += "\t";
                text += doubleToString(writeMesurement.rot_z); text += "\n";
            }
            #else
            text += barPresent; text += ",";
            text += handPresent; text += ",";
            text += floatToString2Decimals(batteryVoltage); text += ",";

            text += String(setting_current_page); text += ",";
            text += floatToString2Decimals(getWeight(charToPage(setting_current_page))); text += ",";

            text += doubleToString(writeMesurement.lin_acc_x); text += ",";
            text += doubleToString(writeMesurement.lin_acc_y); text += ",";
            text += doubleToString(writeMesurement.lin_acc_z); text += ",";

            text += doubleToString(writeMesurement.abs_quat_w); text += ",";
            text += doubleToString(writeMesurement.abs_quat_x); text += ",";
            text += doubleToString(writeMesurement.abs_quat_y); text += ",";
            text += doubleToString(writeMesurement.abs_quat_z); text += ",";

            text += doubleToString(writeMesurement.acc_x); text += ",";
            text += doubleToString(writeMesurement.acc_y); text += ",";
            text += doubleToString(writeMesurement.acc_z); text += ",";
            
            text += doubleToString(writeMesurement.rot_x); text += ",";
            text += doubleToString(writeMesurement.rot_y); text += ",";
            text += doubleToString(writeMesurement.rot_z); text += ",";

            text += doubleToString(writeMesurement.temp); text += ",";
            
            text += doubleToString(writeMesurement.mag_x); text += ",";
            text += doubleToString(writeMesurement.mag_y); text += ",";
            text += doubleToString(writeMesurement.mag_z); text += ",";
            
            text += doubleToString(writeMesurement.abs_height); text += ",";
            
            text += writeMesurement.time_since_last_measure_us; text += "\n";
            #endif
        }
        stringDoneTime= esp_timer_get_time();
        if(!text.isEmpty()){
            logData(text);
        }
        taskDoneTime = esp_timer_get_time();


        /*Serial.print("Total time was: ");
        Serial.print(taskDoneTime-taskSDCardStartTime);
        Serial.print("\t| String creation took: ");
        Serial.print(stringDoneTime-taskSDCardStartTime);
        Serial.print("\t| Write to sd card took: ");
        Serial.print(taskDoneTime-stringDoneTime);
        Serial.print("us\t| Free Space was: ");
        Serial.print(freeSpace);
        Serial.print(" out of ");
        Serial.println(sensorReadingsQueueLen);*/
        
        
        xSemaphoreGive(spiControlMutex);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Setup start");


    Wire.begin(21, 22);         // SDA pin 21, SCL pin 22
    Wire.setClock(400000);      // Set I2C frequency to 400kHz
    Serial.println("Initialising I2C Master: sda=21 scl=22 freq=400000");
    
    setupPiezo();
    setupAnalogSensors();
    setupInputWheel();
    setupDisplay();
    delay(250);
    displayStartupScreen();
    delay(250);


    delay(50);
    bool sd_works = setupSDCard();
    displayStartupState(0, sd_works);
    delay(50);
    bool imc_works = setupICM42688P();
    displayStartupState(1, imc_works);
    delay(50);
    bool mmc_works = setupMMC5983MA();
    displayStartupState(2, mmc_works);
    delay(50);
    bool bmp_works = setupBMP390();
    displayStartupState(3, bmp_works);
    delay(50);
    bool bno_works = setupBNO055();
    displayStartupState(4, bno_works);
    delay(250);


    if(!sd_works || !imc_works|| !mmc_works|| !bmp_works|| !bno_works){
        delay(750);
        displayFailPage("Initializing failed!");
        return;
    }
    Wire.setClock(400000);      // Set I2C frequency to 400kHz


    uint8_t sys, gyro, accel, mag = 0;
    if(!bno055HasCalibrationSettingsStored()){
        bool isCal = bno055IsFullyCalibrate();
        while (!isCal){
            bno055GetCalibration(&sys, &gyro, &accel, &mag);
            displayBNO055CalibrationScreen(sys, gyro, accel, mag);
            isCal = bno055IsFullyCalibrate();
            delay(500);//~2fps
        }

        if(readBNO055OffsetsToSettings()){
            writeSettingsToSDCard();
        }else{
            displayFailPage("Write bno cal failed!");
            return;
        }
    } else {
        writeBNO055OffsetsFromSettings();
        delay(50);
        bno055GetCalibration(&sys, &gyro, &accel, &mag);
        displayBNO055CalibrationScreen(sys, gyro, accel, mag);
        delay(1000);//~2fps
    }


    displayBNO055CalibratedScreen();

    delay(1000);

    if(createNewDataLogFilePath() < 1 ){
        displayFailPage("Wrong file count!");
        return;
    }
    if(!createNewDataLogFile()){
        displayFailPage("Failed file create!");
    }

    displayStartingRecording();    
    
    semTimerReadSensors = xSemaphoreCreateBinary();
    spiControlMutex = xSemaphoreCreateMutex();

    sensorReadingsQueue = xQueueCreate(sensorReadingsQueueLen, sizeof(Measurement_t));

    // 520kByte total available
    xTaskCreatePinnedToCore(
                    taskHandleIO,
                    "taskHandleIO",
                    64*1024,//stack size bytes
                    NULL,
                    1,// Task priority (0 to configMAX_PRIORITIES - 1)
                    &taskHandleHandleIO,
                    core_sd_card_io);
    
     xTaskCreatePinnedToCore(
                    taskSDCard,
                    "taskSDCard",
                    40*1024,//stack size bytes
                    NULL,
                    2,// Task priority (0 to configMAX_PRIORITIES - 1)
                    &taskHandleSDCard,
                    core_sd_card_io);
    
    delayNonBlockingMillis(500);

    xTaskCreatePinnedToCore(
                    taskReadSensors,
                    "taskReadSensors",
                    8*1024,//stack size bytes
                    NULL, 
                    1,// Task priority (0 to configMAX_PRIORITIES - 1)
                    &taskHandleReadSensors,
                    core_sensors);
    
    Serial.println("Setup done");
}

void loop() {
    vTaskDelete(NULL);


    //testBMP390();
    
    //testBNO055();
    //testMMC5983MA();
    //testICM42688P();
    
    //test_logging();
    //serialPrintAnalogSensorTest();
    //serialPrintEncoderTest();
    //setPiezo(500,10000);
    //delay(1000);
    //testDisplay();
    //testBMP390();
    //tft.fillScreen(TFT_BLACK);
    //tft.setCursor(20, 4, 4);
    //tft.setTextColor(TFT_YELLOW);
    //tft.print("Voltage: ");
    //tft.print(readBatteryVoltage());
}
