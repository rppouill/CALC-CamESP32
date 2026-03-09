#ifndef GATTSCHAR_H
#define GATTSCHAR_H

// ESP Includes
#include "esp_gatts_api.h"

// CPP Includes
#include <vector>
#include <deque>

// Project Includes

#define GATTS_CHAR_TAG "GATTCharacteristic"

template<typename T>
class GATTSCharacteristic {

    private:
        inline static uint16_t primary_svc_uuid_ = ESP_GATT_UUID_PRI_SERVICE;
        inline static uint16_t char_decl_uuid_ = ESP_GATT_UUID_CHAR_DECLARE;

        std::vector<esp_gatts_attr_db_t> raw_table_;
        std::deque<uint16_t> uuid_;
        std::vector<uint16_t> handle_;

        inline static uint16_t cccd_uuid_ = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        std::deque<uint16_t> cccd_values_;
        

        
    public:
        GATTSCharacteristic() = default;
        GATTSCharacteristic(uint16_t service_uuid) {
            this->uuid_.push_back(service_uuid);
            ESP_LOGI(GATTS_CHAR_TAG, "Adding characteristic with UUID 0x%x", this->uuid_.back());
            raw_table_.push_back({{ESP_GATT_AUTO_RSP}, 
                {ESP_UUID_LEN_16, (uint8_t *)&primary_svc_uuid_, ESP_GATT_PERM_READ, 
                    sizeof(uint16_t), sizeof(uint16_t), (uint8_t *)&this->uuid_.back()}}); 
        }
        /*void addCharacteristic(uint16_t uuid, uint8_t props, uint16_t max_len, uint8_t* data_ptr) {
            // Declaration
            raw_table_.push_back({{ESP_GATT_AUTO_RSP}, 
                {ESP_UUID_LEN_16, (uint8_t *)&char_decl_uuid_, ESP_GATT_PERM_READ, 
                    (sizeof(uint8_t)), (sizeof(uint8_t)), &props}});
            // Value
            this->uuid_.push_back(uuid);
            ESP_LOGI(GATTS_CHAR_TAG, "Adding characteristic with UUID 0x%04x", this->uuid_.back());
            raw_table_.push_back({{ESP_GATT_AUTO_RSP}, 
                {ESP_UUID_LEN_16, (uint8_t *)&this->uuid_.back(), ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 
                    max_len, 0, data_ptr}});
        }*/
       void addCharacteristic(uint16_t uuid, uint8_t props, uint16_t max_len, uint8_t* data_ptr) {
            // Declaration
            this->raw_table_.push_back({ {ESP_GATT_AUTO_RSP},
                { ESP_UUID_LEN_16, (uint8_t*)&char_decl_uuid_, ESP_GATT_PERM_READ,
                    sizeof(uint8_t),sizeof(uint8_t),&props } });

            uuid_.push_back(uuid);
            ESP_LOGI(GATTS_CHAR_TAG, "Adding characteristic with UUID 0x%04x", this->uuid_.back());
            raw_table_.push_back({{ESP_GATT_AUTO_RSP}, 
                {ESP_UUID_LEN_16, (uint8_t *)&this->uuid_.back(), ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 
                    max_len, 0, data_ptr}});

            /* =========================
            AUTO CCCD (NOTIFY/INDICATE)
            ========================= */
            /*if(props & (ESP_GATT_CHAR_PROP_BIT_NOTIFY |
                        ESP_GATT_CHAR_PROP_BIT_INDICATE))
            {
                cccd_values_.push_back(0x0000);
                raw_table_.push_back({ {ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, (uint8_t*)&cccd_uuid_, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 
                    sizeof(uint16_t), sizeof(uint16_t), (uint8_t*)&cccd_values_.back()} });
                ESP_LOGI(GATTS_CHAR_TAG, "Auto-added CCCD for UUID 0x%04x", uuid_.back());
            }*/
        }
        
        // Getters 
        std::vector<esp_gatts_attr_db_t>& getRawTable() { return raw_table_; }
        
        esp_attr_control_t  getAttrCtrl(uint8_t i) const { return raw_table_[i].attr_control; }
            uint8_t getAutRsp(uint8_t i) const { return raw_table_[i].attr_control.auto_rsp; }
        esp_attr_desc_t     getAttDesc(uint8_t i) const { return raw_table_[i].att_desc; }
            uint16_t getUUIDLength(uint8_t i) const { return raw_table_[i].att_desc.uuid_length; }
            uint8_t* getUUID(uint8_t i) const { return raw_table_[i].att_desc.uuid_p; }
            uint16_t getPerms(uint8_t i) const  { return raw_table_[i].att_desc.perm; }
            uint16_t getMaxLength(uint8_t i) const { return raw_table_[i].att_desc.max_length; }
            uint16_t getLength(uint8_t i) const { return raw_table_[i].att_desc.length; }
            T getValue(uint8_t i) const { return *reinterpret_cast<T*>(raw_table_[i].att_desc.value); }
        uint8_t size(){ return raw_table_.size(); }

        void setHandles(const uint16_t* handles, size_t count) {
            this->handle_.assign(handles, handles + count);
        }  

        std::vector<uint16_t> getHandles() const { return handle_; }



        
};

#endif // GATTSCHARMANAGE_H