#ifndef GATTCMANAGER_H
#define GATTCMANAGER_H

// ESP Includes
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "esp_random.h"

// CPP Includes
#include <memory>
#include <unordered_map>
#include <string>
#include <cstring>

// Project Includes
#include "GAP.h"
#include "Pattern/Singleton/Singleton.h"
#include "Pattern/Observer/IObserver.h"
#include "DataBase.h"
#include "DataBlock.h"


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
            next_app_id_(0),
            remote_device_name_(remote_device_name) {
        } 
        uint16_t allocate_app_id(){
            return this->next_app_id_++;
        }

        uint8_t MAX_CLIENTS;
        uint8_t full_connected_;
        uint8_t stop_scan_done_;
        uint16_t next_app_id_;
        std::unordered_map<esp_gatt_if_t, std::shared_ptr<T>> clients_;
        std::unordered_map<uint16_t, std::shared_ptr<T>> pending_clients_;
        std::string remote_device_name_;


        std::list<IObserver<>*> observers_;


    public:
        static void gattc_callback_bridge(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
            GATTCManager::getInstance().operator()(event, gattc_if, param);
        }
        void operator()(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
            switch(event){
                case ESP_GATTC_REG_EVT: {
                    if(param->reg.status != ESP_GATT_OK){
                        ESP_LOGE(GATTC_MANAGER, 
                                "Failed to register GATTC app, app_id= %x | error code = %x", 
                                param->reg.app_id, param->reg.status);
                        break;
                    }
                    auto it = pending_clients_.find(param->reg.app_id);
                    if(it == pending_clients_.end()){
                        ESP_LOGE(GATTC_MANAGER, "No pending client found for app_id 0x%x", param->reg.app_id);
                    } else {
                        this->clients_[gattc_if] = it->second;
                        this->pending_clients_.erase(it);

                        ESP_LOGI(GATTC_MANAGER, 
                            "Registered GATTC app_id=%x gattc_if=%x for device %s with bda", 
                            param->reg.app_id, gattc_if, 
                            this->clients_[gattc_if]->getRemoteDeviceName().c_str());
                        esp_log_buffer_hex(GATTC_MANAGER, this->clients_[gattc_if]->getRemoteBDA().data(), sizeof(esp_bd_addr_t));

                        uint32_t delay_ms = 100 + (esp_random() % 400);
                        vTaskDelay(pdMS_TO_TICKS(delay_ms));
                        esp_ble_gattc_open(gattc_if, 
                                           this->clients_[gattc_if]->getRemoteBDA().data(), 
                                           this->clients_[gattc_if]->getBleAddrType(), true);
                    }
                    
                    break;
                }
                case ESP_GATTC_UNREG_EVT: {
                    ESP_LOGI(GATTC_MANAGER,
                            "Unregister gattc_if=%d app_id=%d",
                            gattc_if,
                            param->reg.app_id);

                    this->clients_.erase(gattc_if);
                    break;
                }
                case ESP_GATTC_DISCONNECT_EVT: {
                    auto it = this->clients_.find(gattc_if);
                    if(it != this->clients_.end()){
                        it->second->setConnDevice(0); // Mark client as disconnected
                        ESP_LOGI(GATTC_MANAGER, "Client gattc_if %x disconnected, marked as not connected", gattc_if);
                    }
                    break;
                } 
                case ESP_GATTC_OPEN_EVT: {
                    auto it = this->clients_.find(gattc_if);
                    if(it != this->clients_.end()){
                        it->second->onOpen(event, gattc_if, param);
                    }   
                    if(!this->full_connected_){
                        this->Notify();
                    } break;
                }
                default:
                    auto it = this->clients_.find(gattc_if);
                    if(it != this->clients_.end()){
                        (*it->second)(event, gattc_if, param);
                    } 
                    break;
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
                        for(const auto& [gattc_if, client] : this->clients_){
                            if(std::strcmp((char*)adv_name, client->getRemoteDeviceName().c_str()) == 0){
                                flag = false;
                            } 
                        }
                        if(flag){
                            esp_ble_gap_stop_scanning();
                            ESP_LOGE(GATTC_MANAGER, "New Device found: %s", adv_name);
                            auto& database = DataBase::getInstance();
                            database.create_new_block((char*)adv_name, 1024, 10);
                            

                            auto new_client = std::make_shared<T>(
                                                    std::vector<uint16_t>{0x00FF, 0x00FA},
                                                    std::vector<uint16_t>{0xFF01, 0xFA01},
                                                    database[(char*)adv_name]
                                                );
                            uint16_t app_id = this->allocate_app_id();
                            new_client->setAppId(app_id); // App ID will be set properly upon registration, this is just a placeholder
                            ESP_LOGI(GATTC_MANAGER, "Remote BDA:");
                            esp_log_buffer_hex(GATTC_MANAGER, scan_result->scan_rst.bda, sizeof(esp_bd_addr_t));
                            new_client->setRemoteBDA(*(std::array<uint8_t, ESP_BD_ADDR_LEN>*)scan_result->scan_rst.bda);
                            new_client->setRemoteDeviceName((char*)adv_name);
                            new_client->setBleAddrType(scan_result->scan_rst.ble_addr_type);
                            this->pending_clients_[app_id] = new_client;
                            /*if(this->clients_.size() >= this->MAX_CLIENTS){
                                this->full_connected_ = 1;
                                ESP_LOGI(GATTC_MANAGER, "Maximum clients connected, stopping scan");
                            }*/
                            auto ret = esp_ble_gattc_app_register(this->pending_clients_[app_id]->getAppId());
                            if(ret){
                                ESP_LOGE(GATTC_MANAGER, "[%s] Gattc App Register failed, app_id= %x | error code = %x", __func__, this->pending_clients_[app_id]->getAppId(), ret);
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