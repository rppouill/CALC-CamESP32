#include "GATTSImageBroadcaster.h"

#define GATT_IMAGE_TAG(x) gattss_tag(x).c_str()
static inline std::string gattss_tag(uint16_t client_num) {
    return std::format("GATTSImageBroadcaster 0x{:X}", client_num);
}

GATTSImageBroadcaster::GATTSImageBroadcaster(uint16_t service_uuid, std::vector<std::tuple<
                        uint16_t,           // char uuid
                        uint8_t,            // char properties
                        uint16_t,           // char max length    
                        uint8_t*  // char value buffer
                    >> uuid, uint8_t app_id) : GATTService(service_uuid, uuid, app_id) 
{
    this->mutex_ = xSemaphoreCreateMutex();
    
}

void GATTSImageBroadcaster::onRead(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    // TODO
    ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "EXEC_READ_EVT received");
    ESP_LOGW(GATT_IMAGE_TAG(this->service_uuid_), "Not implemented yet !");
}
void GATTSImageBroadcaster::onWrite(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Conn_id 0x%x, trans_id %"PRIu32", handle 0x%x", 
                    param->write.conn_id,
                    param->write.trans_id,
                    param->write.handle);     
    
    auto handles = this->char_.getHandles();
    bool handled = false;

    for (size_t i = 0; i < handles.size(); ++i) {
        uint16_t value_handle = handles[i];

        // Si la caractéristique a un CCCD, il est juste après le handle de valeur
        bool has_cccd = false;
        uint16_t cccd_handle = 0;
        if ((i + 1) < handles.size()) {
            uint16_t next_handle = handles[i + 1];
            uint16_t props = this->char_.getAttDesc(i + 1).perm;
            if (this->char_.getAttDesc(i + 1).uuid_length == ESP_UUID_LEN_16 &&
                *reinterpret_cast<uint16_t*>(this->char_.getAttDesc(i + 1).uuid_p) == ESP_GATT_UUID_CHAR_CLIENT_CONFIG) {
                has_cccd = true;
                cccd_handle = next_handle;
            }
        }

        if (has_cccd && param->write.handle == cccd_handle) {
            uint16_t cccd_value = param->write.value[0] | (param->write.value[1] << 8);
            ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), 
                     "CCCD write for char index %zu: 0x%04x (%s)", 
                     i, cccd_value, cccd_value ? "notifications enabled" : "notifications disabled");

            if (param->write.need_rsp) {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, nullptr);
            }
            handled = true;
            break;
        }
    }
    if (!handled) {
    //if(!param->write.is_prep){
        ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        esp_log_buffer_hex(GATT_IMAGE_TAG(this->service_uuid_), param->write.value, param->write.len);
        
        this->ask_device = std::string((char*)param->write.value, param->write.len);
        ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Received ask_device: %s", this->ask_device.c_str());
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK,NULL);
        }
    }
}
void GATTSImageBroadcaster::onExecWrite(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    // TODO
    ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "EXEC_WRITE_EVT received");
    ESP_LOGW(GATT_IMAGE_TAG(this->service_uuid_), "Not implemented yet");
}
void GATTSImageBroadcaster::onConf(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if(param->conf.status != ESP_GATT_OK){
        ESP_LOGE(GATT_IMAGE_TAG(this->service_uuid_), "Confirmation error, status = %x, attr_handle %d", param->conf.status, param->conf.handle);
    } else {
        //ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Confirmation received successfully, status = %x, attr_handle %d", param->conf.status, param->conf.handle);
        this->ble_serialization_.setBusyTx(0);
    }
}

void GATTSImageBroadcaster::onDisconnect(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
            ESP_LOGI(GATT_SERVICE_TAG(this->service_uuid_), "Device disconnected, conn_id = %d", param->disconnect.conn_id);
            this->conn_id_.erase(param->disconnect.conn_id);
            //this->conn_id_.erase(std::find(this->conn_id_.begin(), this->conn_id_.end(), param->disconnect.conn_id));
            //this->conn_addr_.erase(std::find(this->conn_addr_.begin(), this->conn_addr_.end(), param->disconnect.remote_bda));
            this->conn_device_ = this->conn_id_.size();
            this->adv_controller_.isScanningFalse();
            this->adv_controller_.start_advertising();
            this->ask_device = "";
            this->ble_serialization_.setBusyTx(0);
        }

void GATTSImageBroadcaster::eventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:            this->onReg(event, gatts_if, param);             break;
        case ESP_GATTS_READ_EVT:           this->onRead(event, gatts_if, param);            break;
        case ESP_GATTS_WRITE_EVT:          this->onWrite(event, gatts_if, param);           break;
        case ESP_GATTS_EXEC_WRITE_EVT:     this->onExecWrite(event, gatts_if, param);       break;
        case ESP_GATTS_MTU_EVT:            this->onMtu(event, gatts_if, param);             break;
        case ESP_GATTS_START_EVT:          this->onStart(event, gatts_if, param);           break;
        case ESP_GATTS_CONNECT_EVT:        this->onConnect(event, gatts_if, param);         break;
        case ESP_GATTS_DISCONNECT_EVT:     this->onDisconnect(event, gatts_if, param);      break;
        case ESP_GATTS_CONF_EVT:           this->onConf(event, gatts_if, param);            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT: this->onCreateAttrTab(event, gatts_if, param);   break;
        case ESP_GATTS_CONGEST_EVT:        ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "ESP_GATTS_CONGEST_EVT"); break;
        default: ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Unhandled event: %x", event);                    break;
    }
}

// Specific methods
esp_err_t GATTSImageBroadcaster::sendNotification(uint8_t* data, uint16_t len, uint16_t conn_id, uint8_t char_index) {
    return esp_ble_gatts_send_indicate(this->gatts_if_, conn_id, 
                            this->char_.getHandles()[char_index], 
                            len, data, false);
}

esp_err_t GATTSImageBroadcaster::sendIndication(uint8_t* data, uint16_t len, uint16_t conn_id, uint8_t char_index) {
    return esp_ble_gatts_send_indicate(this->gatts_if_, conn_id, 
                            this->char_.getHandles()[char_index], 
                            len, data, true);
}

esp_err_t GATTSImageBroadcaster::sendImage(uint8_t* data, uint16_t len, uint16_t conn_id) {
    if(this->char_.size() < 7) {
        ESP_LOGE(GATT_IMAGE_TAG(this->service_uuid_), "Characteristic index for image data not found");
        return ESP_FAIL;
    }
    esp_err_t ret = ESP_OK;
    for(auto el_table : this->char_.getRawTable()){
        ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Characteristic max length: %d", el_table.att_desc.max_length);
    }
    ret =  this->sendNotification(data, this->char_.getMaxLength(4), conn_id, 4);
    ret += this->sendNotification(data, this->char_.getMaxLength(6), conn_id, 6);
    
    return ret;
}

esp_err_t GATTSImageBroadcaster::broadcast(uint8_t* data, uint16_t len, uint8_t indicate, uint16_t char_index, std::string filter_device) {
    esp_err_t ret = ESP_OK;
    /*xSemaphoreTake(this->mutex_, portMAX_DELAY);
    for(auto conn_id : this->conn_id_){
        ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Broadcasting to conn_id %x, with size %d", conn_id, len);
        esp_ble_gatts_send_indicate(this->gatts_if_, conn_id, 
                            this->char_.getHandles()[char_index], 
                            len, data, indicate);
        //if(notif) ret = this->sendNotification(data, len, conn_id, char_index);
        //else ret = this->sendIndication(data, len, conn_id, char_index);
    }
    xSemaphoreGive(this->mutex_);*/
    for(auto conn_id : this->conn_id_){
        if(filter_device.empty() || conn_id.second != filter_device){
            ESP_LOGI(GATT_IMAGE_TAG(this->service_uuid_), "Broadcasting to conn_id %x, handle 0x%x, with size %d | MTU = %d", 
                                            conn_id.first, this->char_.getHandles()[char_index], len, this->getMtu());
            this->ble_serialization_.enqueue({
                    this->gatts_if_, 
                    conn_id.first, 
                    this->char_.getHandles()[char_index], 
                    len, data, indicate});
        }
        
    }
    return ret;
}


// Getters and Setters
std::string GATTSImageBroadcaster::getAskDevice() {
    return this->ask_device;
}