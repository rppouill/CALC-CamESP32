#pragma once
#include "esp_gatts_api.h"
#include <cstdint>

class GATTSProfile {
    public:
        GATTSProfile() : gatts_if_(ESP_GATT_IF_NONE), conn_device_(0), id_(0) {}
        GATTSProfile(uint16_t id) : gatts_if_(ESP_GATT_IF_NONE), conn_device_(0), id_(id) {}
        virtual ~GATTSProfile() {}

        virtual void eventHandler(esp_gatts_cb_event_t event, 
                                esp_gatt_if_t gatts_if, 
                                esp_ble_gatts_cb_param_t *param) = 0;

        
        // 
        virtual void sendNotification(uint8_t* data, uint16_t len, uint16_t sender_id) = 0;
        virtual void sendIndication(uint8_t* data, uint16_t len, uint16_t sender_id) = 0;

        // Setters and Getters
        void setGattsIf(uint16_t iface) { gatts_if_ = iface;}
        void setConnId(uint16_t id)     { conn_id_ = id;    }
        
        uint8_t getConnDevice() const { return conn_device_; }
        uint16_t getGattsIf() const { return gatts_if_; }
        uint16_t getConnId() const { return conn_id_; }
        uint16_t getId() const { return id_; }

    protected:
        uint16_t gatts_if_;
        uint16_t conn_id_;
        uint8_t conn_device_;
        uint16_t id_;
};