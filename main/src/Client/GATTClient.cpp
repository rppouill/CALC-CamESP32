#include "GATTClient.h"

GATTClient::GATTClient(uint16_t remote_service_uuid16, uint16_t remote_filter_char_uuid, uint16_t notify_descr_uuid) : 
    gattc_if_(ESP_GATT_IF_NONE), conn_id_(0), service_start_handle_(0), service_end_handle_(0), conn_device_(0),
    remote_filter_service_uuid_{
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = remote_service_uuid16}
    },
    remote_filter_char_uuid_ {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = remote_filter_char_uuid}
    }, 
    notify_descr_uuid_ {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = notify_descr_uuid}
    }{
    //GAP::getInstance().registerObserver(shared_from_this());
}

// Getters
uint16_t GATTClient::getGattcIf()               const { return gattc_if_; }
uint16_t GATTClient::getAppId()                 const { return app_id_; }
uint16_t GATTClient::getConnId()                const { return conn_id_; }
uint16_t GATTClient::getServiceStartHandle()    const { return service_start_handle_; }
uint16_t GATTClient::getServiceEndHandle()      const { return service_end_handle_; }
uint8_t GATTClient::getConnDevice()             const { return conn_device_; }
std::array<uint8_t, ESP_BD_ADDR_LEN> GATTClient::getRemoteBDA()       const { return remote_bda_; }

std::vector<uint16_t> GATTClient::getCharHandles()       const { return char_handle_; }
std::vector<uint16_t> GATTClient::getReadCharAvailable() const { return read_char_available_; }
std::string           GATTClient::getRemoteDeviceName()  const { return remote_device_name_; }
esp_ble_addr_type_t   GATTClient::getBleAddrType()       const { return ble_addr_type_; }

// Setters
void GATTClient::setGattcIf(uint16_t gattc_if)  { gattc_if_ = gattc_if; }
void GATTClient::setAppId(uint16_t app_id)      { app_id_ = app_id; }
void GATTClient::setConnId(uint16_t conn_id)    { conn_id_ = conn_id; }
void GATTClient::setServiceHandles(uint16_t start, uint16_t end) { 
    service_start_handle_ = start; 
    service_end_handle_ = end; 
}
void GATTClient::setConnDevice(uint8_t connected)                       { conn_device_ = connected; }
void GATTClient::setRemoteBDA(std::array<uint8_t, ESP_BD_ADDR_LEN> bda) { remote_bda_ = bda; }
void GATTClient::addCharHandle(uint16_t handle)                         { char_handle_.push_back(handle); }
void GATTClient::addReadCharAvailable(uint16_t handle)                  { read_char_available_.push_back(handle); }
void GATTClient::setRemoteDeviceName(std::string name)                  { remote_device_name_ = name; }
void GATTClient::setBleAddrType(esp_ble_addr_type_t addr_type)          { ble_addr_type_ = addr_type; } 

void GATTClient::operator()(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param) {
    //auto client_num = param->connect.conn_id % MAX_CLIENTS; // Assuming conn_id is assigned sequentially and starts from 0
    //auto client_num = param->reg.app_id;
    switch(event){
        case ESP_GATTC_CONNECT_EVT:      onConnect(event, gattc_if, param); break;
        case ESP_GATTC_OPEN_EVT:         onOpen(event, gattc_if, param); break;
        case ESP_GATTC_DIS_SRVC_CMPL_EVT:onDisSrvcCmpl(event, gattc_if, param); break;
        case ESP_GATTC_CFG_MTU_EVT:      onCfgMtu(event, gattc_if, param); break;
        case ESP_GATTC_SEARCH_RES_EVT:   onSearchRes(event, gattc_if, param); break;
        case ESP_GATTC_SEARCH_CMPL_EVT:  onSearchCmpl(event, gattc_if, param); break;
        case ESP_GATTC_REG_FOR_NOTIFY_EVT:onRegForNotify(event, gattc_if, param); break;
        case ESP_GATTC_NOTIFY_EVT:       onNotify(event, gattc_if, param); break;
        case ESP_GATTC_WRITE_DESCR_EVT:  onWriteDescr(event, gattc_if, param); break;
        case ESP_GATTC_SRVC_CHG_EVT:     onSrvcChg(event, gattc_if, param); break;
        case ESP_GATTC_WRITE_CHAR_EVT:   onWriteChar(event, gattc_if, param); break;
        case ESP_GATTC_READ_CHAR_EVT:    onReadChar(event, gattc_if, param); break;
        case ESP_GATTC_DISCONNECT_EVT:   onDisconnect(event, gattc_if, param); break;
        default: break;
    }
}
void GATTClient::onConnect(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Client number: %d", this->app_id_);
    this->conn_id_ = param->connect.conn_id;
    std::copy(std::begin(param->connect.remote_bda), std::end(param->connect.remote_bda), this->remote_bda_.begin());
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Remote BDA:");
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), this->remote_bda_.data(), sizeof(esp_bd_addr_t));
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), param->connect.remote_bda, sizeof(esp_bd_addr_t));

    this->conn_device_ = 1;
    
    esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, param->connect.conn_id);
    if(mtu_ret){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to send MTU request, error code = 0x%x", mtu_ret); 
    }
}
void GATTClient::onOpen(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
     if (param->open.status != ESP_GATT_OK) {
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "open failed, status %x", param->open.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Connection succes conn_id %d, if %d", param->connect.conn_id, gattc_if);
    }

}
void GATTClient::onDisSrvcCmpl(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->dis_srvc_cmpl.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to disconnect from service, status 0x%x", param->dis_srvc_cmpl.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Disconnected from service 0x%x successfully", param->dis_srvc_cmpl.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &this->remote_filter_service_uuid_);
    }
}
void GATTClient::onCfgMtu(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->cfg_mtu.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to configure MTU, status 0x%x", param->cfg_mtu.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "MTU configured successfully, MTU %d", param->cfg_mtu.mtu);
    }
}
void GATTClient::onSearchRes(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "SEARCH RES: conn_id = %x is primary service %d", param->search_res.conn_id, param->search_res.is_primary);
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "start handle %d end handle %d current handle value %d", param->search_res.start_handle, param->search_res.end_handle, param->search_res.srvc_id.inst_id);
    if(param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && param->search_res.srvc_id.uuid.uuid.uuid16 == this->remote_filter_service_uuid_.uuid.uuid16){
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Found our service UUID: 0x%04x", param->search_res.srvc_id.uuid.uuid.uuid16);
        this->service_start_handle_ = param->search_res.start_handle;
        this->service_end_handle_   = param->search_res.end_handle;
        // get_server = true;
    }
    
}
void GATTClient::onSearchCmpl(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->search_cmpl.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to complete service search, status 0x%x", param->search_cmpl.status);
        return;
    }

    if(param->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE) {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Get service information from remote device");
    } else if (param->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH) {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Get service information from flash");
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "unknown service source");
    }

    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Service search complete: conn_id = %x, searched_service_source = %d", param->search_cmpl.conn_id, param->search_cmpl.searched_service_source);
    if(true){ // TODO get server
        uint16_t count = 0x0000;
        uint8_t status = esp_ble_gattc_get_attr_count(
                gattc_if, 
                param->search_cmpl.conn_id,
                ESP_GATT_DB_CHARACTERISTIC,
                this->service_start_handle_,
                this->service_end_handle_,
                0x00,
                &count);
        if (status != ESP_GATT_OK){
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "[%s] esp_ble_gattc_get_attr_count error status: 0x%x", __func__, status);
        } else {
            ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Number of characteristics found: %d", count);
        }
        if (count > 0){
            std::vector<esp_gattc_char_elem_t> char_elem_result(count);
            ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Start handle %x End handle %x, char count %d",
                                this->service_start_handle_,
                                this->service_end_handle_,
                                count);
            status = esp_ble_gattc_get_char_by_uuid(
                    gattc_if,
                    param->search_cmpl.conn_id,
                    this->service_start_handle_,
                    this->service_end_handle_,
                    this->remote_filter_char_uuid_,
                    char_elem_result.data(),
                    &count);
            
            if(status != ESP_GATT_OK){
                ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "[%s] esp_ble_gattc_get_char_by_uuid error status: 0x%x", __func__, status);
            }
            ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Number of characteristics with matching UUID found: %d", count);
            
            for(size_t i = 0; i < count; i++){
                ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Characteristic %d: handle %d, properties %x, uuid len %d, uuid %x", i, char_elem_result[i].char_handle, char_elem_result[i].properties, char_elem_result[i].uuid.len, char_elem_result[i].uuid.uuid.uuid16);
                status += esp_ble_gattc_get_all_char(
                    gattc_if, 
                    param->search_cmpl.conn_id,
                    this->service_start_handle_,
                    this->service_end_handle_,
                    &char_elem_result[i],
                    &count, i);
                
                ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "char_elem_result[%d].properties = %x, uuid = %x", i, char_elem_result[i].properties, char_elem_result[i].uuid.uuid.uuid16);
            }

            if(status != ESP_GATT_OK){
                ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "[%s] esp_ble_gattc_get_all_char error status: 0x%x", __func__, status);
            } else {
                for(size_t i = 0; i < count; i++){
                    if(char_elem_result[i].properties & (ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ)){
                        this->char_handle_.push_back(char_elem_result[i].char_handle);
                        esp_ble_gattc_register_for_notify(
                            gattc_if,
                            this->remote_bda_.data(),
                            char_elem_result[i].char_handle
                        );
                        this->read_char_available_.push_back(char_elem_result[i].char_handle);
                        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Found characteristic %x", i);
                    }
                }
            }
        } else {
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "No characteristics found");
        }
    }

}
void GATTClient::onRegForNotify(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Registered for notification, status %d, handle %d", 
        param->reg_for_notify.status,
        param->reg_for_notify.handle);
    
    if(param->reg_for_notify.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to register for notification, status 0x%x", param->reg_for_notify.status);
    } else {
        uint16_t count = 0x0000;
        auto ret_status = esp_ble_gattc_get_attr_count(
                gattc_if, 
                this->conn_id_,
                ESP_GATT_DB_DESCRIPTOR,
                this->service_start_handle_,
                this->service_end_handle_,
                this->char_handle_[0],
                &count);
        if(ret_status != ESP_GATT_OK){
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "[%s] esp_ble_gattc_get_attr_count error status: 0x%x", __func__, ret_status);
        }

        if(count > 0){
            std::vector<esp_gattc_descr_elem_t> descr_elem_result(count);
            ret_status = esp_ble_gattc_get_descr_by_char_handle(
                    gattc_if,
                    this->conn_id_,
                    param->reg_for_notify.handle,
                    this->notify_descr_uuid_,
                    descr_elem_result.data(),
                    &count);
            ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Found %d descriptors", count);
            if(ret_status != ESP_GATT_OK){
                ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "esp_ble_gattc_get_descr_by_char_handle error");
            }
        } else {
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "No descriptors found");
        }
    }
}
void GATTClient::onNotify(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){ 
    //this->onNotify(event, gattc_if, param, this->app_id_);
}
void GATTClient::onWriteDescr(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    //this->onWriteDescr(event, gattc_if, param, this->app_id_);
}
void GATTClient::onSrvcChg(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Service change indication for remote BDA:");
            esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), param->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
}
void GATTClient::onWriteChar(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->write.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to write characteristic, status 0x%x", param->write.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Wrote characteristic successfully, handle %d", param->write.handle);
    }
}
void GATTClient::onReadChar(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    //this->onReadChar(event, gattc_if, param, this->app_id_);
}
void GATTClient::onDisconnect(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    this->conn_device_ = 0;
}



