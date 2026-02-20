#include "GATTSImageBroadcaster.h"

template<typename T>
GATTSImageBroadcaster<T>::GATTSImageBroadcaster() : GATTSProfile(), adv_config_done_(0), peer_gatts_addr{0} {

    if(this->device_name_.empty()) {
        const uint8_t * addr = esp_bt_dev_get_address();
        std::array<char, 20> buf;
        snprintf(buf.data(), buf.size(), "CALC-Cam%02X%02X%02X", 
                addr[3], addr[4], addr[5]);
        this->device_name_ = buf.data();
    }

    this->init();
    
    
}

template<typename T>
GATTSImageBroadcaster<T>::GATTSImageBroadcaster(const std::string& device_name) : GATTSProfile(), device_name_(device_name), adv_config_done_(0), peer_gatts_addr{0} {
    this->init();
}

template<typename T>
void GATTSImageBroadcaster<T>::init() {
    static uint8_t service_uuid128[16] = {
        /* LSB <--------------------------------------------------------------------------------> MSB */
        //first uuid, 16bit, [12],[13] is the value
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00,
    };
    this->adv_data_ = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data =  NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 16,
        .p_service_uuid = service_uuid128,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    //std::memcpy(this->adv_data_.p_service_uuid, service_uuid128, 16);
    this->scan_rsp_data_ = {
        .set_scan_rsp = true,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0x00,/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
        .manufacturer_len = 0,
        .p_manufacturer_data =  NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = 0,
        .p_service_uuid = NULL,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    
    auto current_block = DataBase::getInstance().get_data_block(this->device_name_);
    
    this->service_manager = std::make_shared<GATTCharacteristics>(0x00FF);
    this->service_manager->add(0xFF02, ESP_GATT_CHAR_PROP_BIT_NOTIFY, 512, current_block.get()->operator[](current_block->get_read_block()).data());
    this->service_manager->add(0xFF03, ESP_GATT_CHAR_PROP_BIT_NOTIFY, 512, current_block.get()->operator[](current_block->get_read_block()).data() + 512);
    this->service_manager->add(0xFF01, ESP_GATT_CHAR_PROP_BIT_WRITE, 11, nullptr);

}

template<typename T>
void GATTSImageBroadcaster<T>::eventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
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
        case ESP_GATTS_OPEN_EVT:           this->onOpen(event, gatts_if, param);            break;
        default: ESP_LOGI(GATT_IMAGE_TAG, "Unhandled event: %x", event);                    break;
    }
}

template<typename T>
void GATTSImageBroadcaster<T>::onReg(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
    auto ret = esp_ble_gap_set_device_name(this->device_name_.c_str());
    if(ret){
        ESP_LOGE(GATT_IMAGE_TAG, "set device name failed, error code = %x", ret);
    }
    // Config Advertising Data
    ret = esp_ble_gap_config_adv_data(&adv_data_);
    if (ret) {
        ESP_LOGE(GATT_IMAGE_TAG, "config adv data failed, error code = %x", ret);
    }
    this->adv_config_done_ |= (1 << 0);
    // Config Scan Response Data
    ret = esp_ble_gap_config_adv_data(&scan_rsp_data_);
    if (ret) {
        ESP_LOGE(GATT_IMAGE_TAG, "config scan rsp data failed, error code = %x", ret);
    }
    this->adv_config_done_ |= (1 << 1);
    // Done: Create Services
    ret = this->buildAttributes(gatts_if);
    if (ret){
        ESP_LOGE(GATT_IMAGE_TAG, "create attr table failed, error code = %x", ret);
    }
}
template<typename T>
void GATTSImageBroadcaster<T>::onRead(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    // TODO
    ESP_LOGI(GATT_IMAGE_TAG, "EXEC_READ_EVT received");
    ESP_LOGW(GATT_IMAGE_TAG, "Not implemented yet !");
}
template<typename T>
void GATTSImageBroadcaster<T>::onWrite(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(GATT_IMAGE_TAG, "conn_id 0x%x, trans_id %"PRIu32", handle 0x%x", 
                    param->write.conn_id,
                    param->write.trans_id,
                    param->write.handle);
    if(!param->write.is_prep){
        ESP_LOGI(GATT_IMAGE_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        esp_log_buffer_hex(GATT_IMAGE_TAG, param->write.value, param->write.len);
        // Copy the received data to the ask_device string
        this->ask_device = std::string((char*)param->write.value, param->write.len);
        ESP_LOGI(GATT_IMAGE_TAG, "Received ask_device: %s", this->ask_device.c_str());
        if (param->write.need_rsp) {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK,NULL);
        }
    }
}
template<typename T>
void GATTSImageBroadcaster<T>::onExecWrite(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    // TODO
    ESP_LOGI(GATT_IMAGE_TAG, "EXEC_WRITE_EVT received");
    ESP_LOGW(GATT_IMAGE_TAG, "Not implemented yet");
}
template<typename T>
void GATTSImageBroadcaster<T>::onMtu(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(GATT_IMAGE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
}
template<typename T>
void GATTSImageBroadcaster<T>::onStart(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(GATT_IMAGE_TAG, "SERVICE_START_EVT, status 0x%x, service_handle 0x%x",
                    param->start.status, param->start.service_handle);
}
template<typename T>
void GATTSImageBroadcaster<T>::onConnect(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(GATT_IMAGE_TAG, "Peer GATTS Addr: %s", this->peer_gatts_addr);
    this->conn_id_ = param->connect.conn_id;
    this->conn_device_ = 1;
    ESP_LOGI(GATT_IMAGE_TAG, "Device connected, conn_id set to %d", this->conn_id_);
    ESP_LOGI(GATT_IMAGE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id 0x%x, remote %02x:%02x:%02x:%02x:%02x:%02x",
                    param->connect.conn_id,
                    param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                    param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
    ESP_LOGI(GATT_IMAGE_TAG, "Connected to %02X%02X%02X", param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
            
}
template<typename T>
void GATTSImageBroadcaster<T>::onDisconnect(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    ESP_LOGI(GATT_IMAGE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason 0x%x", param->disconnect.reason);
    this->conn_device_ = 0;
    this->Notify();
    
}
template<typename T>
void GATTSImageBroadcaster<T>::onConf(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if(param->conf.status != ESP_GATT_OK){
        ESP_LOGI(GATT_IMAGE_TAG, "Indication confirmation error, status = %d", param->conf.status);
    }
}       
template<typename T>
void GATTSImageBroadcaster<T>::onCreateAttrTab(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    if (param->add_attr_tab.status != ESP_GATT_OK) {
        ESP_LOGE(GATT_IMAGE_TAG, "Create attribute table failed, error code= 0x%x", param->add_attr_tab.status);
    }  else if(param->add_attr_tab.num_handle != this->service_manager->getGattDbSize()) {
        ESP_LOGE(GATT_IMAGE_TAG, "Create attribute table abnormally, num_handle (%d) \
                 doesn't equal to the number of attributes (%d)",
                 param->add_attr_tab.num_handle,
                 this->service_manager->getGattDbSize());
    } else {
        ESP_LOGI(GATT_IMAGE_TAG, "Create attribute table successfully, the number handle = 0x%x", param->add_attr_tab.num_handle);
        
        this->service_manager->mapHandles(param->add_attr_tab.handles);
        esp_ble_gatts_start_service(param->add_attr_tab.handles[0]);
    }
}
template<typename T>
void GATTSImageBroadcaster<T>::onOpen(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    // Wesh Alors !
}



template<typename T>
void GATTSImageBroadcaster<T>::broadcast(uint8_t* data, uint16_t len, uint16_t sender_id) {
    // TO DO
}

template<typename T>
void GATTSImageBroadcaster<T>::sendIndication(uint8_t* data, uint16_t len, uint16_t sender_id) {
    esp_ble_gatts_send_indicate(this->gatts_if_, 
                                this->conn_id_, 
                                this->service_manager->operator[](sender_id).getHandle(),
                                len, data, false);
}

template<typename T>
void GATTSImageBroadcaster<T>::sendNotification(uint8_t* data, uint16_t len, uint16_t sender_id) {
    // TO DO
}

template<typename T>
void GATTSImageBroadcaster<T>::sendImage(uint16_t len, uint16_t sender_id) {
    auto current_block = DataBase::getInstance().get_data_block(this->device_name_);
    if(!this->ask_device.empty()) {
        current_block = DataBase::getInstance().get_data_block(this->ask_device);
    }

    if(len > IMAGE_SIZE) {
        ESP_LOGE(GATT_IMAGE_TAG, "Image size exceeds buffer capacity. Max size: %d", IMAGE_SIZE);
    } else if(len != CHARACTERISTIC_IMAGE_SIZE * 2) {
        ESP_LOGW(GATT_IMAGE_TAG, "Image size is not optimal. Expected size: %d", CHARACTERISTIC_IMAGE_SIZE * 2);
    } else {
        this->sendIndication(current_block.get()->operator[](current_block->get_read_block()).data()                           , CHARACTERISTIC_IMAGE_SIZE, 0);
        this->sendIndication(current_block.get()->operator[](current_block->get_read_block()).data() + CHARACTERISTIC_IMAGE_SIZE, CHARACTERISTIC_IMAGE_SIZE, 1);
    }


}

template<typename T>
esp_err_t GATTSImageBroadcaster<T>::buildAttributes(esp_gatt_if_t gatts_if) {
    auto raw_db = this->service_manager->generateRawDb();

    auto ret = esp_ble_gatts_create_attr_tab(
        raw_db.data(), 
        gatts_if, 
        raw_db.size(), 
        0
    );

    return ret;
}

//template class GATTSImageBroadcaster<uint8_t[]>;
//template class GATTSImageBroadcaster<DoubleBuffer<uint8_t>>;
template class GATTSImageBroadcaster<DataBlock<std::vector<uint8_t>>>;