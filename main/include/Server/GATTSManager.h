#ifndef GATTSMANAGER_H
#define GATTSMANAGER_H

// ESP Includes
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

// CPP Includes
#include <unordered_map>
#include <vector>
#include <string>
#include <tuple>
#include <memory>

// Project Includes
#include "Pattern/Singleton/Singleton.h"
#include "AdvertisingController.h"


#define GATTS_MANAGER "GATTSManager"

template <typename T, 
          typename P>
class GATTSManager : public Singleton<GATTSManager<T,P>> {
    friend class Singleton<GATTSManager<T,P>>;

    private:
        GATTSManager() : adv_controller_(AdvertisingController::getInstance()) {}
        GATTSManager(std::string device_name, 
                esp_ble_adv_data_t adv_data, 
                esp_ble_adv_data_t scan_rsp_data) : device_name_(device_name), 
                adv_data_(adv_data), scan_rsp_data_(scan_rsp_data), adv_controller_(AdvertisingController::getInstance()) {

        }

        std::unordered_map<uint16_t, std::shared_ptr<T>> services_;


        std::string device_name_;
        esp_ble_adv_data_t adv_data_;
        esp_ble_adv_data_t scan_rsp_data_;
        AdvertisingController& adv_controller_;



    public: 
        static void gatts_callback_bridge(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
            GATTSManager::getInstance().operator()(event, gatts_if, param);
        }
        void operator()(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            switch(event) {
                case ESP_GATTS_REG_EVT: {
                    if(param->reg.status != ESP_GATT_OK){
                        ESP_LOGE(GATTS_MANAGER, "Reg app failed, app_id %04x, status %d",
                                    param->reg.app_id,
                                    param->reg.status);
                        break;
                    } 
                    auto service = this->services_.find(param->reg.app_id);
                    if(service == this->services_.end()){
                        ESP_LOGE(GATTS_MANAGER, "Failed to find service for registered app_id %x", param->reg.app_id);
                        return;
                    }
                    auto ret = esp_ble_gap_set_device_name(this->device_name_.c_str());
                    if(ret){
                        ESP_LOGE(GATTS_MANAGER, "Set device name failed, error code = %x", ret);
                    }

                    service->second->setGattsIf(gatts_if);
                    ESP_LOGI(GATTS_MANAGER, "Registered GATTS app_id=%x gatts_if=%x", param->reg.app_id, gatts_if);
                    
                    ret = esp_ble_gap_config_adv_data(&this->adv_data_);
                    if(ret){
                        ESP_LOGE(GATTS_MANAGER, "Config adv data failed, error code = %x", ret);
                    }
                    this->adv_controller_.setAdvConfigDone(this->adv_controller_.getAdvConfigDone() | (1 << 0));
                    ret = esp_ble_gap_config_adv_data(&this->scan_rsp_data_);
                    if(ret){
                        ESP_LOGE(GATTS_MANAGER, "Config scan response data failed, error code = %x", ret);
                    }
                    this->adv_controller_.setAdvConfigDone(this->adv_controller_.getAdvConfigDone() | (1 << 1));
                    break;
                }
                case ESP_GATTS_UNREG_EVT: {
                    auto service = this->services_.find(param->reg.app_id);
                    if(service != this->services_.end()){
                        ESP_LOGI(GATTS_MANAGER, "Unregistered GATTS app_id=%x gatts_if=%x", param->reg.app_id, service->second->getGattsIf());
                    } else {
                        ESP_LOGE(GATTS_MANAGER, "Failed to find service for unregistered app_id %x", param->reg.app_id);
                    }
                break;
                }
                default:
                    break; 
            }
            
            for(auto& service : this->services_){
                if( gatts_if == ESP_GATT_IF_NONE ||
                    service.second->getGattsIf() == gatts_if){
                    service.second->eventHandler(event, gatts_if, param);
                }
            }
        }
        void create_service(uint16_t service_uuid, std::vector<std::tuple<
                        uint16_t,           // char uuid
                        uint8_t,            // char properties
                        uint16_t,           // char max length    
                        uint8_t*  // char value buffer
                    >> uuid){
            this->services_[service_uuid] = std::make_shared<T>(service_uuid, uuid, this->services_.size());
        }
        esp_err_t register_service(){
            esp_err_t ret = ESP_OK;
            for(auto& service_uuid : this->services_){
                ret += esp_ble_gatts_app_register(service_uuid.first);
                if(!ret){
                    ESP_LOGI(GATTS_MANAGER, "GATTS Service created successfully with uuid %04x", service_uuid.first);
                }
            }
            return ret;
        }
        std::shared_ptr<T> operator[](uint16_t service_uuid){
            auto service = this->services_.find(service_uuid);
            if(service != this->services_.end()){
                return service->second;
            } else {
                ESP_LOGE(GATTS_MANAGER, "Failed to find service for uuid %x", service_uuid);
                return nullptr;
            }
        }

};
#endif // GATTSMANAGER_H