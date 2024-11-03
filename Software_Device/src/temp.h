#include <Arduino.h>
#include "pins.h"

#define CORE_SD_Card_IO 0 //PRO_CPU  core 0 is wifi/bt core by default //has special watchdog if scheduler isn't triggered in long time -> resets microcontroller (so wireless is not stuck in loop)
#define CORE_SENSORS    1 //APP_CPU  core 1 is app core by default


static const uint16_t timer_divider = 240;          // Divide 240 MHz by this -> new clock rate is 1mhz
//ESP.getCpuFreqMHz()
static const uint64_t timer_max_count = 1000;  // Timer counts to this value

static hw_timer_t *myhwtimer = NULL;
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;

static SemaphoreHandle_t bin_sem_timer_defered = NULL;

TaskHandle_t taskBlink1Handle;
static QueueHandle_t msg_queue;


static SemaphoreHandle_t mutex;


static TimerHandle_t autoreload_timer = NULL;


void IRAM_ATTR onTimer(){

    BaseType_t task_woken = pdFALSE;
    xSemaphoreGiveFromISR(bin_sem_timer_defered, &task_woken);//useTaskNotify instead?
    if (task_woken) {
        portYIELD_FROM_ISR();
    }

    portENTER_CRITICAL(&spinlock); // disable hw interrupts
    portEXIT_CRITICAL(&spinlock);

    vTaskSuspendAll();
    xTaskResumeAll();
}


void testTaskfromhardwareTimer(TimerHandle_t xTimer){
    while(1) {
        xSemaphoreTake(bin_sem_timer_defered, portMAX_DELAY);
        //do stuff
    }
}


void myTimerCallback(TimerHandle_t xTimer) {
    Serial.println("Timer expired");
    //(uint32_t)pvTimerGetTimerID(xTimer) -> timer id
}

void toggleLED_2(void *parameter) {
    while(1) {
        // do stuff here
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // Give amount of free heap memory (uncomment if you'd like to see it)
        // Serial.print("Free heap (bytes): ");
        // Serial.println(xPortGetFreeHeapSize());
        // Serial.print("Stack high water mark (words (4bytes /32bit)):");
        // Serial.println(uxTaskGetStackHighWaterMark(NULL));
        //int *ptr = (int*)pvPortMalloc(1024*sizeof(int));


        //xTaskGetTickCount() -> current time since startup in ticks
        //vPortFree(ptr);
        //xTaskGetTickCount() = millis();
        //esp_timer_get_time(); = micros();? The time passed since the initialization of ESP Timer -> start of app_main
        ESP.getCpuFreqMHz();
        int item;
        if (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE) {
            //Serial.println(item);
        }
        Serial.println(item);

        // Wait before trying again
        vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE){
            //stuff
            xSemaphoreGive(mutex);
        }
        //xPortGetCoreID();


}

void setup() {
    int msg_queue_len = 10;
    msg_queue = xQueueCreate(msg_queue_len, sizeof(int));

    mutex = xSemaphoreCreateMutex();

    bin_sem_timer_defered = xSemaphoreCreateBinary();


    autoreload_timer = xTimerCreate(  // Use xTaskCreate() in vanilla FreeRTOS
                "auto reloader",  // timer name
                2000/ portTICK_PERIOD_MS, //period of timer in ticks
                pdTRUE,         // auto reload
                (void *) 0,            // timer ID
                myTimerCallback);         //callback function
    if(autoreload_timer == NULL){
        Serial.println("could not create timer");
    }

    xTimerStart(autoreload_timer, portMAX_DELAY);//wait time if timer queue is full

    // Task to run forever
    xTaskCreatePinnedToCore(  // Use xTaskCreate() in vanilla FreeRTOS
                toggleLED_2,  // Function to be called
                "Toggle 1",   // Name of task
                1024,         // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,         // Parameter to pass to function
                1,            // Task priority (0 to configMAX_PRIORITIES - 1)
                &taskBlink1Handle,         // Task handle
                CORE_SD_Card_IO);     // Run on one core for demo purposes (ESP32 only)



    myhwtimer = timerBegin(0, timer_divider, true);//use esptimer
    timerAttachInterrupt(myhwtimer, &onTimer, true);
    timerAlarmWrite(myhwtimer, timer_max_count, true);
    timerAlarmEnable(myhwtimer);





// Serial.println("\n##################################");
// Serial.println(F("ESP32 Information:"));
// Serial.printf("Internal Total Heap %d, Internal Used Heap %d, Internal Free Heap %d\n", ESP.getHeapSize(), ESP.getHeapSize()-ESP.getFreeHeap(), ESP.getFreeHeap());
// Serial.printf("Sketch Size %d, Free Sketch Space %d\n", ESP.getSketchSize(), ESP.getFreeSketchSpace());
// Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
// Serial.printf("Chip Model %s, ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
// Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
// Serial.println("##################################\n");

}

void loop() {
    static int num  = 0;
    if (xQueueSend(msg_queue, (void *)&num, 10) != pdTRUE) {//pdFALSE
        Serial.println("Queue full");
    }
    vTaskDelete(NULL);
}