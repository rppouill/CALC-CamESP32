#pragma once
#include "GATTCharacteristics.h"


GATTCharacteristics::GATTCharacteristics(uint16_t service_uuid) : service_uuid_(service_uuid), gatt_db_size_(1) 
{}

void GATTCharacteristics::add(uint16_t uuid, uint8_t props, uint16_t max_len, uint8_t* buffer) {
    list_.emplace_back(uuid, props, max_len);
    buffers_.push_back(buffer);
    this->gatt_db_size_ += 2;
}

void GATTCharacteristics::build(std::vector<esp_gatts_attr_db_t>& db) {
    db.push_back({{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_svc_uuid_, ESP_GATT_PERM_READ, 
            sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&service_uuid_}});
    for (size_t i = 0; i < list_.size(); ++i) {
        list_[i].addToTable(db, buffers_[i]);
    }
}
std::vector<esp_gatts_attr_db_t> GATTCharacteristics::generateRawDb() {
    std::vector<esp_gatts_attr_db_t> raw_db;
    raw_db.push_back({{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_svc_uuid_, ESP_GATT_PERM_READ, 
            sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&service_uuid_}});

    for (size_t i = 0; i < list_.size(); ++i) {
        list_[i].fillRawEntries(raw_db, buffers_[i]);
    }
    return raw_db;
}

void GATTCharacteristics::mapHandles(const uint16_t* handles) {
    for (size_t i = 0; i < list_.size(); ++i) {
        list_[i].setHandle(handles[2 + (i * 2)]);
    }
}

// Operator Overload
GATTCharacteristic& GATTCharacteristics::operator[](size_t idx) { 
    return list_.at(idx); 
}


// Getters / Setters
size_t GATTCharacteristics::getGattDbSize() const { 
    return gatt_db_size_; 
}