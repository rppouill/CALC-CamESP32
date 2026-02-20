#ifndef GATTCMANAGER_H
#define GATTCMANAGER_H

#include "GAP.h"
#include "Pattern/Singleton/Singleton.h"
#include "Pattern/Observer/IObserver.h"
#include "DataBase.h"
#include "DataBlock.h"

#include <memory>
#include <vector>
#include <string>
#include <cstring>

#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"

#define GATTC_MANAGER "GATTCManager"
#define GAP_CLIENT_TAG "GATTCManager - GAP"

template <typename T>
class GATTCManager : public Singleton<GATTCManager<T>>, 
                     public IObserver<esp_ble_gap_cb_param_t *>, 
                     public ISubject<IObserver<>> {
    friend class Singleton<GATTCManager<T>>;

    private:
        GATTCManager(std::string remote_device_name = "CALC") : 
            MAX_CLIENTS(5),
            full_connected_(0),
            stop_scan_done_(0),
            remote_device_name_(remote_device_name) {
        } 

        uint8_t MAX_CLIENTS;
        uint8_t full_connected_;
        uint8_t stop_scan_done_;
        std::vector<std::shared_ptr<T>> clients_;
        std::string remote_device_name_;

        std::list<IObserver<>*> observers_;


    public:
        static void gattc_callback_bridge(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
            GATTCManager::getInstance().operator()(event, gattc_if, param);
        }
        void operator()(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {

            switch(event){
                case ESP_GATTC_REG_EVT: {
                    if(param->reg.status == ESP_GATT_OK){
                        this->clients_[param->reg.app_id]->setGattcIf(gattc_if);
                        ESP_LOGI(GATTC_MANAGER, "Registered GATTC app_id 0x%x with gattc_if: 0x%x", param->reg.app_id, gattc_if); 
                    } else {
                        ESP_LOGW(GATTC_MANAGER, "Failed to register GATTC app_id %04x, status 0x%x", param->reg.app_id, param->reg.status);
                    } break;
                }
                case ESP_GATTC_UNREG_EVT: {
                    ESP_LOGI(GATTC_MANAGER, "Unregistered GATTC app_id 0x%x", param->reg.app_id);
                    break;
                }
                case ESP_GATTC_DISCONNECT_EVT: {
                    esp_ble_gattc_app_unregister(this->clients_[param->reg.app_id]->getAppId());
                    this->clients_.erase(this->clients_.begin() + param->reg.app_id);
                    break;
                } 
                case ESP_GATTC_OPEN_EVT: {
                    if(!this->full_connected_){
                        this->Notify();
                    } break;
                }
                default: break;
            }
            ESP_LOGI(GATTC_MANAGER, "GATTC event %x received for gattc_if %x", event, gattc_if);
            for(auto& client : this->clients_){
                if(client->getGattcIf() == ESP_GATT_IF_NONE || 
                   client->getGattcIf() == gattc_if){
                    (*client)(event, gattc_if, param);
                    ESP_LOGD(GATTC_MANAGER, "Client Gattc_if | %x", client->getGattcIf());
                    ESP_LOGD(GATTC_MANAGER, "       Gattc_if | %x", gattc_if);
                    ESP_LOGD(GATTC_MANAGER, "       Event    | %x", event);
                    ESP_LOGD(GATTC_MANAGER, "Client ConnDev  | %x", client->getConnDevice());
                    
                    if(!client->getConnDevice() && event == ESP_GATTC_REG_EVT){
                        ESP_LOGE(GATTC_MANAGER, "Connection to :");
                        esp_log_buffer_hex(GATTC_MANAGER, client->getRemoteBDA().data(), sizeof(esp_bd_addr_t));
                        esp_ble_gattc_open(client->getGattcIf(),
                                           client->getRemoteBDA().data(),
                                           client->getBleAddrType(), true);                                       
                    }
                }
            }
            
        }

        uint8_t getMaxClients() const { return MAX_CLIENTS; }

        // IObervable interface
        void Update(esp_ble_gap_cb_param_t * const &param) override {
            /*  
            // OnScanResult of GAP will call this Update function to notify GATTClient of new scan results.
            */
            auto scan_result = (esp_ble_gap_cb_param_t*) param;
            switch(scan_result->scan_rst.search_evt){
                case ESP_GAP_SEARCH_INQ_RES_EVT : {
                    if(this->full_connected_) break; // If already fully connected, ignore scan results
                    uint8_t adv_name_len = 0;
                    uint8_t* adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
                    if(adv_name_len > 0) adv_name[adv_name_len] = '\0'; // Null-terminate the device name string
                    if(adv_name && std::strstr((char*)adv_name, this->remote_device_name_.c_str())){
                        auto flag = true;
                        for(auto client : this->clients_){
                            if(std::strcmp((char*)adv_name, client->getRemoteDeviceName().c_str()) == 0){
                                flag = false;
                            } 
                        }
                        if(flag){
                            esp_ble_gap_stop_scanning();
                            ESP_LOGE(GATTC_MANAGER, "New Device found: %s - Vector client size: %x", adv_name, this->clients_.size());
                            auto& database = DataBase::getInstance();
                            database.create_new_block((char*)adv_name, 1024, 10);
                            auto new_client = std::make_shared<T>(0x00FF, 0xFF02, database.get_data_block((char*)adv_name));
                            new_client->setAppId(this->clients_.size()); // App ID will be set properly upon registration, this is just a placeholder
                            new_client->setRemoteBDA(*(std::array<uint8_t, ESP_BD_ADDR_LEN>*)scan_result->scan_rst.bda);
                            new_client->setRemoteDeviceName((char*)adv_name);
                            new_client->setBleAddrType(scan_result->scan_rst.ble_addr_type);
                            this->clients_.push_back(new_client);
                            if(this->clients_.size() >= this->MAX_CLIENTS){
                                this->full_connected_ = 1;
                                ESP_LOGI(GATTC_MANAGER, "Maximum clients connected, stopping scan");
                            }
                            auto ret = esp_ble_gattc_app_register(this->clients_.back()->getAppId());
                            if(ret){
                                ESP_LOGE(GATTC_MANAGER, "[%s] Gattc App Register failed, app_id= %x | error code = %x", __func__, this->clients_.back()->getAppId(), ret);
                            } else {
                                ESP_LOGI(GATTC_MANAGER, "Registered GATTC app_id %x for device %s with bda", this->clients_.back()->getAppId(), adv_name); 
                                esp_log_buffer_hex(GATTC_MANAGER, this->clients_.back()->getRemoteBDA().data(), sizeof(esp_bd_addr_t));
                            }
                        }
                    }
                    break;
                }
                case ESP_GAP_SEARCH_INQ_CMPL_EVT : {
                    ESP_LOGI(GAP_CLIENT_TAG, "Scan stop !");
                    break;
                }
                default: break;
            }

        }

         // ISubject interface
        void Attach(IObserver<> *observer) override     { observers_.push_back(observer); }
        void Detach(IObserver<> *observer) override     { observers_.remove(observer);    }
        void Notify() override                          { for (auto observer : observers_) observer->Update(); } 
};

#endif // GATTCMANAGER_H