#include "GATTCImageReceiver.h"


GATTCImageReceiver::GATTCImageReceiver(uint16_t remote_service_uuid16, uint16_t remote_filter_char_uuid, std::shared_ptr<DataBlock<std::vector<uint8_t>>> data_block, uint16_t notify_descr_uuid) : 
    GATTClient(remote_service_uuid16, remote_filter_char_uuid, notify_descr_uuid), data_block_(data_block), parity_flag_(0) {}

/*void GATTCImageReceiver::onNotify(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->notify.is_notify){
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Notification received from handle %d, value len %d", 
            param->notify.handle,
            param->notify.value_len);

        if(param->notify.value_len == 512 && !this->parity_flag_){
            this->parity_flag_ = !this->parity_flag_;
            std::copy(param->notify.value, param->notify.value + param->notify.value_len, this->image_buffer_.begin());
        } else if(param->notify.value_len == 512 && this->parity_flag_){ // TO DO: Better management of image chunks BECAUSE It's ASSUMED that only 2 chunks of 512 bytes are sent
            this->parity_flag_ = !this->parity_flag_;
            std::copy(param->notify.value, param->notify.value + param->notify.value_len, this->image_buffer_.begin() + 512);
        } 
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Indication received from handle %d, value len %d", 
            param->notify.handle,
            param->notify.value_len);
    }
}*/

void GATTCImageReceiver::onNotify(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->notify.is_notify){
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Notification received from handle %d, value len %d", 
            param->notify.handle,
            param->notify.value_len);
        static std::vector<uint8_t> image_buffer(1024, 0);
        if(param->notify.value_len == 512 && !this->parity_flag_){
            this->parity_flag_ = !this->parity_flag_;
            std::copy(param->notify.value, param->notify.value + param->notify.value_len, image_buffer.begin());
        } else if(param->notify.value_len == 512 && this->parity_flag_){ // TO DO: Better management of image chunks BECAUSE It's ASSUMED that only 2 chunks of 512 bytes are sent
            this->parity_flag_ = !this->parity_flag_;
            std::copy(param->notify.value, param->notify.value + param->notify.value_len, image_buffer.begin() + 512);
            this->data_block_->push_back(image_buffer);
        } 
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Indication received from handle %d, value len %d", 
            param->notify.handle,
            param->notify.value_len);
    }
}

void GATTCImageReceiver::onWriteDescr(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->write.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to write descriptor, status 0x%x", param->write.status);
    } else {
        // Write Something if needed
    }
}

void GATTCImageReceiver::onReadChar(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->read.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to read characteristic, status 0x%x", param->read.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Read characteristic value len %d", param->read.value_len);
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Value: ");
        esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), param->read.value, param->read.value_len);
        
    }
}