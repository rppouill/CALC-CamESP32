#ifndef GATTSERVICE_H
#define GATTSERVICE_H

// ESP Includes
#include "esp_gatts_api.h"
#include "esp_log.h"

// CPP Includes
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <format>
#include <unordered_map>

// Project Includes
#include "Characteristic/GATTSCharacteristic.h"
#include "AdvertisingController.h"


#define GATT_SERVICE_TAG(x) gatts_tag(x).c_str()
static inline std::string gatts_tag(uint16_t client_num) {
    return std::format("GATTService 0x{:X}", client_num);
}
template<typename T>
class GATTService {
    private: 
        //std::vector<esp_bd_addr_t>  conn_addr_;

        

    protected:
        uint16_t service_uuid_;
        uint8_t app_id_;
        uint8_t conn_device_;
        uint16_t mtu;

        AdvertisingController& adv_controller_;

        //std::vector<uint16_t>       conn_id_;
        std::unordered_map<uint16_t, std::string> conn_id_; 
        esp_gatt_if_t gatts_if_;
        GATTSCharacteristic<T> char_;

        virtual void onReg(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
            auto ret = esp_ble_gatts_create_attr_tab(
                this->char_.getRawTable().data(),
                gatts_if,
                this->char_.getRawTable().size(),
                0
            );
            if(ret){
                ESP_LOGE(GATT_SERVICE_TAG(this->service_uuid_), "Failed to create attribute table, error code = %x", ret);
            }
        }
        virtual void onMtu(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "MTU updated, new MTU = %d", param->mtu.mtu);
            this->mtu = param->mtu.mtu;
            if(!this->conn_id_.empty()) {
                this->conn_device_ = this->conn_id_.size();
            }
        }
        virtual void onStart(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "Service started, service_handle = 0x%x", param->start.service_handle);
        }
        virtual void onConnect(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            //this->conn_id_.push_back(param->connect.conn_id);
            this->conn_id_[param->connect.conn_id] = 
                std::format("CALC-{:02X}{:02X}{:02X}", 
                    param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
            //this->conn_addr_.push_back(param->connect.remote_bda);
            if(this->mtu == 517) {
                this->conn_device_ = this->conn_id_.size();
            }
            ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "Device connected, conn_id = %s", this->conn_id_[param->connect.conn_id].c_str());
            ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "ESP_GATTS_CONNECT_EVT, conn_id 0x%x, remote %02x:%02x:%02x:%02x:%02x:%02x",
                    param->connect.conn_id,
                    param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                    param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
            
            this->adv_controller_.isScanningFalse();
            this->adv_controller_.start_advertising();
        }
        virtual void onDisconnect(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "Device disconnected, conn_id = %d", param->disconnect.conn_id);
            this->conn_id_.erase(param->disconnect.conn_id);
            //this->conn_id_.erase(std::find(this->conn_id_.begin(), this->conn_id_.end(), param->disconnect.conn_id));
            //this->conn_addr_.erase(std::find(this->conn_addr_.begin(), this->conn_addr_.end(), param->disconnect.remote_bda));   
            this->conn_device_ = this->conn_id_.size();
            this->adv_controller_.isScanningFalse();
            this->adv_controller_.start_advertising();
        }
        virtual void onCreateAttrTab(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            if(param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATT_SERVICE_TAG(this->service_uuid_), "Failed to create attribute table, error code = %x", param->add_attr_tab.status);
            } else if(param->add_attr_tab.num_handle != this->char_.size()){
                ESP_LOGE(GATT_SERVICE_TAG(this->service_uuid_), "Create attribute table abnormally, num_handle (%d) \
                        doesn't equal to the number of attributes (%d)",
                        param->add_attr_tab.num_handle,
                        this->char_.size());
            } else {
                ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "Attribute table created successfully, num_handle = %d", param->add_attr_tab.num_handle);
                this->char_.setHandles(param->add_attr_tab.handles, param->add_attr_tab.num_handle);
                esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
            }
            

        }
        virtual void onConf(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            if(param->conf.status != ESP_GATT_OK){
                ESP_LOGE(GATT_SERVICE_TAG(this->service_uuid_), "Confirmation error, status = %x, attr_handle %d", param->conf.status, param->conf.handle);
            }
        }
    
        virtual void onRead     (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) = 0;
        virtual void onWrite    (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) = 0;
        virtual void onExecWrite(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) = 0;
        
        virtual void eventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) = 0;
    
    public: 

        GATTService() : service_uuid_(0), app_id_(0), conn_device_(0), mtu(23), adv_controller_(AdvertisingController::getInstance()) {}
        GATTService(uint16_t service_uuid, 
                    std::vector<std::tuple<
                        uint16_t,           // char uuid
                        uint8_t,            // char properties
                        uint16_t,           // char max length    
                        uint8_t*  // char value buffer
                    >> uuid, uint8_t app_id) : service_uuid_(service_uuid), app_id_(app_id), conn_device_(0), mtu(23), adv_controller_(AdvertisingController::getInstance()) {
            this->char_ = GATTSCharacteristic<T>(service_uuid);
            for(auto characteristic : uuid){
                this->char_.addCharacteristic(
                    std::get<0>(characteristic), 
                    std::get<1>(characteristic), 
                    std::get<2>(characteristic), 
                    std::get<3>(characteristic));
            }
        }


        // Getters and Setters
        uint16_t getGattsIf() const { return gatts_if_; }
        uint8_t  getAppId()   const { return app_id_; }
        uint8_t  getConnDevice() const { return conn_device_; }
        uint16_t getMtu() const { return mtu; }

        void setGattsIf(uint16_t gatts_if) { gatts_if_ = gatts_if; }
        




};


#endif // GATTSERVICE_H