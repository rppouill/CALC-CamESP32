#ifndef GATTCLIENT_H
#define GATTCLIENT_H

#include <vector>
#include <array>
#include <string>
#include <cstring>
#include <list>

#include "esp_gattc_api.h"
#include "esp_log.h"

#include "Pattern/Observer/IObserver.h"
#include "Pattern/Observer/ISubject.h"

#define GATTC_CLIENT_TAG(x) gattc_tag(x).c_str()
static inline std::string gattc_tag(uint8_t client_num) {
    return "GATTC_CLIENT_" + std::to_string(client_num);
}

class GATTClient {
    private: 
        // States 
        void onConnect      (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onOpen         (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onDisSrvcCmpl  (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onCfgMtu       (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onSearchRes    (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onSearchCmpl   (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onRegForNotify (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        //void onNotify       (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param, uint8_t client_num);
        //void onWriteDescr   (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param, uint8_t client_num);
        void onSrvcChg      (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        void onWriteChar    (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        //void onReadChar     (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param, uint8_t client_num);
        void onDisconnect   (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);



    protected:
        // Common GATT Client attributes (equivalent to a gattc_profile_inst in ESP-IDF examples)
        uint16_t gattc_if_;
        uint16_t app_id_;
        uint16_t conn_id_;
        uint16_t service_start_handle_;
        uint16_t service_end_handle_;
        std::vector<uint16_t> char_handle_;
        std::vector<uint16_t> read_char_available_;
        std::array<uint8_t, ESP_BD_ADDR_LEN> remote_bda_;
        uint8_t conn_device_;

        esp_ble_addr_type_t ble_addr_type_;

        std::string remote_device_name_;
        esp_bt_uuid_t remote_filter_service_uuid_;
        esp_bt_uuid_t remote_filter_char_uuid_;
        esp_bt_uuid_t notify_descr_uuid_;
        


    public:
        GATTClient(uint16_t remote_service_uuid16, uint16_t remote_filter_char_uuid, uint16_t notify_descr_uuid);
        void operator()(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
        
        virtual void onNotify    (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) = 0;
        virtual void onWriteDescr(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) = 0;
        virtual void onReadChar  (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) = 0;

        // Getters 
        uint16_t        getGattcIf()            const;
        uint16_t        getAppId()              const;
        uint16_t        getConnId()             const;
        uint16_t        getServiceStartHandle() const;
        uint16_t        getServiceEndHandle()   const;
        uint8_t         getConnDevice()         const;
        std::array<uint8_t, ESP_BD_ADDR_LEN> getRemoteBDA() const;
        std::vector<uint16_t> getCharHandles()       const;
        std::vector<uint16_t> getReadCharAvailable() const;

        std::string getRemoteDeviceName() const;
        esp_ble_addr_type_t getBleAddrType() const;

        // Setters
        void setGattcIf(uint16_t gattc_if);              
        void setAppId(uint16_t app_id);                  
        void setConnId(uint16_t conn_id);                
        void setServiceHandles(uint16_t start, uint16_t end);
        void setConnDevice(uint8_t connected);           
        void setRemoteBDA(std::array<uint8_t, ESP_BD_ADDR_LEN> bda);            

        void addCharHandle(uint16_t handle);
        void addReadCharAvailable(uint16_t handle);

        void setRemoteDeviceName(std::string name);
        void setBleAddrType(esp_ble_addr_type_t addr_type);


       

};

#endif // GATTCLIENT_H