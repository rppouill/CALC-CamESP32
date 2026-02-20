#pragma once
#include "esp_gatts_api.h"
#include "esp_log.h"

#include "Pattern/Singleton/Singleton.h"
#include "GATTSImageBroadcaster.h"

#include <memory>
#include <vector>
#define GATT_SERVICE_TAG "GATTService"


template<typename T>
class GATTService : public Singleton<GATTService<T>> {
    friend class Singleton<GATTService>;

    private:
        GATTService() = default;
        GATTService(std::vector<std::shared_ptr<class GATTSImageBroadcaster<T>>> profiles) : profiles_(profiles) {
            num_services_ = profiles.size();
        }

        int num_services_;
        std::vector<std::shared_ptr<class GATTSImageBroadcaster<T>>> profiles_;


    public:
        static void gatts_callback_bridge(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
            GATTService::getInstance().operator()(event, gatts_if, param);
        }



        void operator()(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            /* If event is register event, store the gatts_if for each profile */
            if (event == ESP_GATTS_REG_EVT) {
                if (param->reg.status == ESP_GATT_OK) {
                    profiles_[param->reg.app_id]->setGattsIf(gatts_if);
                } else {
                    ESP_LOGI(GATT_SERVICE_TAG, "Reg app failed, app_id %04x, status %d",
                                param->reg.app_id,
                                param->reg.status);
                    return;
                }
            }

            for(auto profile : profiles_) {
                if(gatts_if == ESP_GATT_IF_NONE || gatts_if == profile->getGattsIf()) {
                    //if(profile->eventHandler) {
                        profile->eventHandler(event, gatts_if, param);
                    //}
                }
            }
        }
        


};