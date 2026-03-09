#pragma once

#include <string>
#include <memory>
#include <vector>
#include <list>
#include <algorithm>

#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

#include "Pattern/Singleton/Singleton.h"
#include "Pattern/Observer/ISubject.h"
#include "Pattern/Observer/IObserver.h"
#include "AdvertisingController.h"


#define GAP_TAG "GAP"
class GAP : public Singleton<GAP>,
            public IObserver<>,
            public ISubject<IObserver<esp_ble_gap_cb_param_t *>> {
                
    friend class Singleton<GAP>;
    private:
        GAP();
        GAP(std::string remote_device_name, esp_ble_adv_params_t adv_params);

        // Methods
        void operator()(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onAdvDataSetCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onScanRspDataSetCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onAdvStartCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onAdvStopCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onUpdateConnParams(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onScanStopCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onScanParamSetCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onScanStartCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onScanResult(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
        void onSearchInqCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

        // Members
        esp_ble_adv_params_t adv_params_;
        std::string remote_device_name_;
        uint8_t is_scanning_;
        AdvertisingController& adv_controller_;
        
        

        std::list<IObserver<esp_ble_gap_cb_param_t *>*> observers_;


    public:
        static void gap_callback_bridge(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

        void startScanning(uint8_t duration_sec); 
        void stopScanning();

        void startAdvertising();

        // IObserver interface
        void Update() override;
        // ISubject interface
        void Attach(IObserver<esp_ble_gap_cb_param_t *> *observer) override;
        void Detach(IObserver<esp_ble_gap_cb_param_t *> *observer) override;
        void Notify() override;
};

