#pragma once
#include "GATTCharacteristic.h"



GATTCharacteristic::GATTCharacteristic(uint16_t uuid, uint8_t props, uint16_t max_len) : uuid_(uuid), props_(props), max_len_(max_len), handle_(0) 
{}

void GATTCharacteristic::addToTable(std::vector<esp_gatts_attr_db_t>& db, uint8_t* data_ptr) {
    // Declaration
    db.push_back({{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&decl_uuid, ESP_GATT_PERM_READ, 
                sizeof(uint8_t), sizeof(uint8_t), &props_}});
    // Value
    db.push_back({{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&uuid_, 
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 
                max_len_, 0, data_ptr}});
}

void GATTCharacteristic::fillRawEntries(std::vector<esp_gatts_attr_db_t>& raw_table, uint8_t* buffer) {
    raw_table.push_back({{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&decl_uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), &props_}});
    raw_table.push_back({{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&uuid_, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, max_len_, 0, buffer}});
}

void GATTCharacteristic::setHandle  (uint16_t handle) { handle_ = handle; }

uint16_t GATTCharacteristic::getHandle()    const { return handle_; }
uint16_t GATTCharacteristic::getUuid()      const { return   uuid_; }

