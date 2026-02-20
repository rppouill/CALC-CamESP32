#ifndef GATTCIMAGERECEIVER_H
#define GATTCIMAGERECEIVER_H
#include <array>

#include "GATTClient.h"
#include "DataBase.h"
#include "DataBlock.h"

#include "esp_gap_ble_api.h"
#include "esp_log.h"

class GATTCImageReceiver : public GATTClient {
    public:
        GATTCImageReceiver(uint16_t remote_service_uuid16, uint16_t remote_filter_char_uuid, std::shared_ptr<DataBlock<std::vector<uint8_t>>> data_block, uint16_t notify_descr_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG);

        void onNotify    (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
        void onWriteDescr(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;
        void onReadChar  (esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) override;

        //void onScanResult(esp_ble_gap_cb_param_t* param); // Notify callback for GAP scan results, to be registered with GAP
    private:
        //std::array<uint8_t, 1024> image_buffer_;
        std::shared_ptr<DataBlock<std::vector<uint8_t>>> data_block_;
        uint8_t parity_flag_;
};

#endif // GATTCIMAGERECEIVER_H