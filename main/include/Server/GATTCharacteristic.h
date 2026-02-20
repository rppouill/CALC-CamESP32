#pragma once
#include "esp_gatts_api.h"
#include <vector>

class GATTCharacteristic {
    public:
        GATTCharacteristic(uint16_t uuid, uint8_t props, uint16_t max_len);
        
        void addToTable(std::vector<esp_gatts_attr_db_t>& db, uint8_t* data_ptr);
        void fillRawEntries(std::vector<esp_gatts_attr_db_t>& raw_table, uint8_t* buffer);

        void setHandle(uint16_t handle);
        uint16_t getHandle()    const;
        uint16_t getUuid()      const;

    private:
        uint16_t uuid_;
        uint8_t props_;
        uint16_t max_len_;
        uint16_t handle_;
        static inline uint16_t decl_uuid = ESP_GATT_UUID_CHAR_DECLARE;
};