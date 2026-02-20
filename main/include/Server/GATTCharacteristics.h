#pragma once
#include "GATTCharacteristic.h"
#include <vector>

class GATTCharacteristics {
    public:
        GATTCharacteristics(uint16_t service_uuid);
        
        std::vector<esp_gatts_attr_db_t> generateRawDb() ;

        void add(uint16_t uuid, uint8_t props, uint16_t max_len, uint8_t* buffer);
        void build(std::vector<esp_gatts_attr_db_t>& db);
        void mapHandles(const uint16_t* handles);
        std::vector<GATTCharacteristic>& getHandles() { return list_; }
        GATTCharacteristic& operator[](size_t idx);

        // GETTERS / SETTERS

        size_t getGattDbSize() const;

        
    private:
        uint16_t service_uuid_;
        std::vector<GATTCharacteristic> list_;
        std::vector<uint8_t*> buffers_;
        size_t gatt_db_size_; 
        inline static uint16_t primary_svc_uuid_ = ESP_GATT_UUID_PRI_SERVICE;
};