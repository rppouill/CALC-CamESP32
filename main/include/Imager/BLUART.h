#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include <vector>

#include "DataBlock.h"
#include "Pattern/Singleton/Singleton.h"

#define BLUART_TAG "BLUART"


#define TXD     (16)
#define RXD     (17)
#define RTS     static_cast<gpio_num_t>(0)
#define CTS     static_cast<gpio_num_t>(1)

#define PORT_NUM    (UART_NUM_1)
#define BAUDRATE    (115200)
#define BUF_SIZE    (1024)

class BLUART : public Singleton<BLUART> {
    friend class Singleton<BLUART>;
    public:
        static void task_wrapper(void *pvParameter);
        void operator()(void* pvParameter);

        SemaphoreHandle_t getSyncSemaphore();

    private:
        BLUART(std::shared_ptr<DataBlock<std::vector<uint8_t>>> data_block = nullptr);
        // State Machine
        void onData(std::vector<uint8_t>& dtmp);
        void onFifoOvf();
        void onBufferFull();
        void onBreak();
        void onParityErr();
        void onFrameErr();
        void onPatternDetect(std::vector<uint8_t>& dtmp);

        uint8_t parity_flag;
        uint16_t idx;
        uart_event_t event;
        QueueHandle_t uart_queue;


        std::shared_ptr<DataBlock<std::vector<uint8_t>>> data_block_;
        SemaphoreHandle_t syncSemaphore;
};