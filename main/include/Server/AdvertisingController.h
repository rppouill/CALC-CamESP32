#ifndef ADVERTISINGCONTROLLER_H
#define ADVERTISINGCONTROLLER_H

// ESP Includes
#include "esp_gatt_common_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_log.h"

// CPP Includes
#include <vector>
#include <list>
#include <memory>
#include <algorithm>    

// Project Includes
#include "Pattern/Singleton/Singleton.h"

class AdvertisingController : public Singleton<AdvertisingController> {
    friend class Singleton<AdvertisingController>;

    private:
        uint8_t adv_config_done_;
        esp_ble_adv_params_t adv_params_;
        uint8_t is_advertising_;

        AdvertisingController() = default;
        AdvertisingController(esp_ble_adv_params_t adv_params) : adv_config_done_(0), adv_params_(adv_params), is_advertising_(0) {}

        
        
    public:

        // War Machine (not State Machine)
        void start_advertising(){
            if (this->adv_config_done_ == 0 && !this->is_advertising_){
                this->is_advertising_ = 1;
                esp_ble_gap_start_advertising(&this->adv_params_);
               
            }
        }
        void stop_advertising(){
            esp_ble_gap_stop_advertising();
            this->is_advertising_ = 0;
        }

        void isScanningFalse() {
            this->is_advertising_ = 0;
        }

        // Getters and Setters
        uint8_t getAdvConfigDone() const { return this->adv_config_done_; }
        void setAdvConfigDone(uint8_t adv_config_done) { this->adv_config_done_ = adv_config_done; }


};
#endif // ADVERTISINGCONTROLLER_H