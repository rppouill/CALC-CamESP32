#ifndef GATTSIMAGEBROADCAST_H
#define GATTSIMAGEBROADCAST_H

// ESP Includes
#include "esp_gatts_api.h"
#include "esp_log.h"

// CPP Includes
#include <vector>
#include <list>
#include <memory>
#include <algorithm>

// Project Includes
#include "GATTService.h"
#include "DataBase.h"
#include "DataBlock.h"
#include "BLESerialization.h"

class GATTSImageBroadcaster : public GATTService<DataBlock<std::vector<uint8_t>>> {
    private:
        std::string ask_device;
        SemaphoreHandle_t mutex_;

        BLESerialization& ble_serialization_ = BLESerialization::getInstance();

    // Event handlers
        void onRead         (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) override;
        void onWrite        (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) override;
        void onExecWrite    (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) override;
        void onConf         (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) override;
        void onDisconnect   (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) override;

    public:
        GATTSImageBroadcaster(uint16_t service_uuid, std::vector<std::tuple<
                        uint16_t,           // char uuid
                        uint8_t,            // char properties
                        uint16_t,           // char max length    
                        uint8_t*  // char value buffer
                    >> uuid, uint8_t app_id);
        void eventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) override;

        // Specific methods
        esp_err_t sendNotification(uint8_t* data, uint16_t len, uint16_t conn_id, uint8_t char_index);
        esp_err_t sendIndication  (uint8_t* data, uint16_t len, uint16_t conn_id, uint8_t char_index);

        esp_err_t sendImage(uint8_t* data, uint16_t len, uint16_t conn_id);
        esp_err_t broadcast(uint8_t* data, uint16_t len, uint8_t notif, uint16_t char_index, std::string filter_device = "");

        // Getters and Setters
        std::string getAskDevice();
        
};
#endif