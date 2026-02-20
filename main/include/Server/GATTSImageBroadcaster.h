#pragma once
// CPP Include
#include <algorithm>
#include <string>
#include <cstring>
#include <array>
#include <memory>
#include <vector>
#include <list>


// ESP Include
#include "esp_log.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"


// Project Include
#include "DataBlock.h"
#include "DataBase.h"
#include "GATTSProfile.h"
#include "GATTCharacteristics.h"
#include "Pattern/Observer/ISubject.h"
#include "Pattern/Observer/IObserver.h"


#define CHARACTERISTIC_IMAGE_SIZE    512
#define IMAGE_SIZE                  1024 

#define GATT_IMAGE_TAG "GATTSImageBroadcaster"

template<typename T>
class GATTSImageBroadcaster : public GATTSProfile, public ISubject<IObserver<>>{

    private:
        void init();
        void sendNotification(uint8_t* data, uint16_t len, uint16_t sender_id) override;
        void sendIndication(uint8_t* data, uint16_t len, uint16_t sender_id) override;

        esp_err_t buildAttributes(esp_gatt_if_t gatts_if);
        void broadcast(uint8_t* data, uint16_t len, uint16_t sender_id);
        
        // State Machine (not War Machine)
        void onReg            (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onRead           (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onWrite          (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onExecWrite      (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onMtu            (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onStart          (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onConnect        (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onDisconnect     (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onConf           (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onCreateAttrTab  (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
        void onOpen           (esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


        // Attributes
        std::string device_name_;
        //std::shared_ptr<T> img_buffer_;
        uint8_t adv_config_done_;
        esp_bd_addr_t peer_gatts_addr;
        

        esp_ble_adv_data_t adv_data_;
        esp_ble_adv_data_t scan_rsp_data_;
        std::shared_ptr<GATTCharacteristics> service_manager;

        std::list<IObserver<>*> observers_;

        std::string ask_device;

    public:
        GATTSImageBroadcaster();
        GATTSImageBroadcaster(const std::string& device_name);
        

        void eventHandler(esp_gatts_cb_event_t event, 
                        esp_gatt_if_t gatts_if, 
                        esp_ble_gatts_cb_param_t *param) override;
        
        void sendImage(uint16_t len, uint16_t sender_id);

        std::shared_ptr<GATTCharacteristics> getServiceManager() const { return service_manager; }


        void Attach(IObserver<> *observer) override     { observers_.push_back(observer); }
        void Detach(IObserver<> *observer) override     { observers_.remove(observer);    }
        void Notify() override                          { for (auto observer : observers_) observer->Update(); } 


    

};