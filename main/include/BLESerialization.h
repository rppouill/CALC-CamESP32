#ifndef BLESERIALIZATION_H
#define BLESERIALIZATION_H

// ESP Includes
#include "esp_gatts_api.h"
#include "esp_log.h"

    // FreeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

// CPP Includes


// Project Includes
#include "Pattern/Singleton/Singleton.h"

class BLESerialization : public Singleton<BLESerialization> {
    friend class Singleton<BLESerialization>;
    private:
        QueueHandle_t queue_;
        TaskHandle_t task_handle_;
        uint8_t busy_tx;


        BLESerialization(uint16_t queue_length = 30) : busy_tx(0) { 
            this->queue_ = xQueueCreate(queue_length, sizeof(TxMessage));
            if(this->queue_ == nullptr){
                ESP_LOGE("BLESerialization", "Failed to create queue");
            } else {
                ESP_LOGI("BLESerialization", "Queue created successfully");
            }
            xTaskCreate(BLESerialization::txTask, "BLE_TX_TASK", 4096, this, 10, &this->task_handle_);
        }
        

    public:
        struct TxMessage {
            esp_gatt_if_t gatts_if;
            uint16_t conn_id;
            uint16_t handle;
            uint16_t len;
            uint8_t* data;
            uint8_t indicate;
        };

        void enqueue(const TxMessage& msg){
            if(xQueueSend(this->queue_, &msg, portMAX_DELAY) != pdPASS){
                ESP_LOGE("BLESerialization", "Failed to enqueue message");
            } 
        }

        static void txTask(void* pvParameters){
            auto instance = static_cast<BLESerialization*>(pvParameters);
            TxMessage msg;
            while(1){
                if(xQueueReceive(instance->queue_, &msg, portMAX_DELAY) == pdPASS){
                    if(!instance->isBusyTx()){
                        instance->busy_tx = 1;
                        esp_ble_gatts_send_indicate(msg.gatts_if, msg.conn_id, msg.handle, msg.len, msg.data, msg.indicate);
                    }
                } else {
                    ESP_LOGE("BLESerialization", "Failed to receive message from queue");
                }
                vTaskDelay(15 / portTICK_PERIOD_MS);
            }
        }
    
        void setBusyTx(uint8_t busy) { this->busy_tx = busy; }

        uint8_t              isBusyTx() const { return this->busy_tx; }
        TaskHandle_t*   getTaskHandle()       { return &task_handle_; }



};

#endif // BLESERIALIZATION_H