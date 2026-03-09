#include "GATTClient.h"

GATTClient::GATTClient(std::vector<uint16_t> remote_service_uuid16, std::vector<uint16_t> remote_filter_char_uuid,  uint16_t notify_descr_uuid) : 
    gattc_if_(ESP_GATT_IF_NONE), conn_id_(0), conn_device_(0),
    notify_descr_uuid_ {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = notify_descr_uuid}
    }
    {
    for(auto uuid16 : remote_service_uuid16){
        esp_bt_uuid_t service_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = uuid16}
        };
        this->remote_filter_service_uuid_.push_back(service_uuid);
    }
    for(auto uuid16 : remote_filter_char_uuid){
        esp_bt_uuid_t char_uuid = {
            .len = ESP_UUID_LEN_16,
            .uuid = {.uuid16 = uuid16}
        };
        this->remote_filter_char_uuid_.push_back(char_uuid);
    }

    /*remote_filter_service_uuid_{
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = remote_service_uuid16}
    },
    remote_filter_char_uuid_ {
        .len = ESP_UUID_LEN_16,
        .uuid = {.uuid16 = remote_filter_char_uuid}
    }, */
    
    //GAP::getInstance().registerObserver(shared_from_this());
}

// Getters
uint16_t GATTClient::getGattcIf()               const { return gattc_if_; }
uint16_t GATTClient::getAppId()                 const { return app_id_; }
uint16_t GATTClient::getConnId()                const { return conn_id_; }
//uint16_t GATTClient::getServiceStartHandle()    const { return service_start_handle_; }
//uint16_t GATTClient::getServiceEndHandle()      const { return service_end_handle_; }
uint16_t GATTClient::getServiceStartHandle()    const { return this->services_.empty() ? 0 : this->services_.back().start; }
uint16_t GATTClient::getServiceEndHandle()      const { return this->services_.empty() ? 0 : this->services_.back().end; }
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
    this->services_.push_back({start, end});
    //service_start_handle_ = start; 
    //service_end_handle_ = end;
    
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
    /*this->conn_id_ = param->connect.conn_id;
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), this->remote_bda_.data(), sizeof(esp_bd_addr_t));
    std::copy(std::begin(param->connect.remote_bda), std::end(param->connect.remote_bda), this->remote_bda_.begin());
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Remote BDA:");
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), this->remote_bda_.data(), sizeof(esp_bd_addr_t));
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), param->connect.remote_bda, sizeof(esp_bd_addr_t));

    this->conn_device_ = 1;
    
    esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, param->connect.conn_id);
    if(mtu_ret){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to send MTU request, error code = 0x%x", mtu_ret); 
    }*/
}
void GATTClient::onOpen(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
     if (param->open.status != ESP_GATT_OK) {
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "open failed, status %x", param->open.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Connection succes conn_id %d, if %d", param->connect.conn_id, gattc_if);
    
    this->conn_id_ = param->connect.conn_id;
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), this->remote_bda_.data(), sizeof(this->remote_bda_));
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), param->connect.remote_bda, sizeof(param->connect.remote_bda));
    //std::copy(std::begin(param->connect.remote_bda), std::end(param->connect.remote_bda), this->remote_bda_.begin());
    //std::memcpy(this->remote_bda_.data(), param->connect.remote_bda, ESP_BD_ADDR_LEN);
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Remote BDA:");
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), this->remote_bda_.data(), sizeof(this->remote_bda_));
    esp_log_buffer_hex(GATTC_CLIENT_TAG(this->app_id_), param->connect.remote_bda, sizeof(param->connect.remote_bda));

    this->conn_device_ = 1;
    
    esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, param->connect.conn_id);
    if(mtu_ret){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to send MTU request, error code = 0x%x", mtu_ret); 
    }
    }
}
void GATTClient::onDisSrvcCmpl(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->dis_srvc_cmpl.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Discover service failed, status 0x%x", param->dis_srvc_cmpl.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Discover service complete conn_id %x", param->dis_srvc_cmpl.conn_id);
        //esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, &this->remote_filter_service_uuid_);
        esp_ble_gattc_search_service(gattc_if, param->dis_srvc_cmpl.conn_id, NULL);
    }
}
void GATTClient::onCfgMtu(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->cfg_mtu.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to configure MTU, status 0x%x", param->cfg_mtu.status);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "MTU configured successfully, MTU %d", param->cfg_mtu.mtu);
    }
}

uint8_t GATTClient::isTargetService(const esp_bt_uuid_t& uuid) {
    for(const auto& target : remote_filter_service_uuid_)
    {
        if(uuid.len == target.len &&
           uuid.uuid.uuid16 == target.uuid.uuid16) {
            return 1;
        }
    }
    return 0;
}
void GATTClient::onSearchRes(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    /*ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "SEARCH RES: conn_id = %x is primary service %d", param->search_res.conn_id, param->search_res.is_primary);
    ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "start handle %d end handle %d current handle value %d", param->search_res.start_handle, param->search_res.end_handle, param->search_res.srvc_id.inst_id);
    if(param->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && param->search_res.srvc_id.uuid.uuid.uuid16 == this->remote_filter_service_uuid_.uuid.uuid16){
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Found our service UUID: 0x%04x", param->search_res.srvc_id.uuid.uuid.uuid16);
        this->service_start_handle_ = param->search_res.start_handle;
        this->service_end_handle_   = param->search_res.end_handle;
        // get_server = true;
    }*/
   auto& uuid = param->search_res.srvc_id.uuid;

    if(uuid.len == ESP_UUID_LEN_16 &&
       isTargetService(uuid))
    {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Found matching service %04x",
                 uuid.uuid.uuid16);

        this->services_.push_back({
            param->search_res.start_handle,
            param->search_res.end_handle
        });
    }
    
}
void GATTClient::onSearchCmpl(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    if(param->search_cmpl.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Service search failed: 0x%x", param->search_cmpl.status);
        return;
    }

        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Service search complete: conn_id = %x, searched_service_source = %d", param->search_cmpl.conn_id, param->search_cmpl.searched_service_source);


    auto handle_exists = [&](uint16_t handle)->uint8_t {
        for(auto h : char_handle_)
            if(h == handle) return 1;
        return 0;
    };

    for(const auto& svc : services_)
    {
        auto start_handle = svc.start;
        auto end_handle   = svc.end;
        uint16_t count = 0x0000;
        esp_err_t status = esp_ble_gattc_get_attr_count(
            gattc_if,
            param->search_cmpl.conn_id,
            ESP_GATT_DB_CHARACTERISTIC,
            start_handle,
            end_handle,
            0x00,
            &count
        );
        if(status != ESP_GATT_OK || count == 0){
            ESP_LOGW(GATTC_CLIENT_TAG(this->app_id_), "No characteristics in service 0x%x-0x%x", start_handle, end_handle);
            continue;
        }

        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Service 0x%x-0x%x → %d chars", start_handle, end_handle, count);
        std::vector<esp_gattc_char_elem_t> chars(count);
        status = esp_ble_gattc_get_all_char(
            gattc_if,
            param->search_cmpl.conn_id,
            start_handle,
            end_handle,
            chars.data(),
            &count,
            0
        );

        if(status != ESP_GATT_OK){
            ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "get_all_char failed: 0x%x", status);
            continue;
        }

        // ===== Iterate characteristics =====
        for(uint16_t i = 0; i < count; ++i) {
            auto &c = chars[i];
            uint16_t uuid16 = 0;
            if(c.uuid.len == ESP_UUID_LEN_16)
                uuid16 = c.uuid.uuid.uuid16;

            ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Char handle=0x%x uuid=0x%04x props=0x%x", c.char_handle, uuid16, c.properties);
            
            // ---------- avoid duplicates ----------
            if(handle_exists(c.char_handle))
                continue;

            char_handle_.push_back(c.char_handle);
            read_char_available_.push_back(c.char_handle);

            // ---------- register notify ----------
            if(c.properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "Register notify handle 0x%x", c.char_handle);

                esp_ble_gattc_register_for_notify(
                    gattc_if,
                    remote_bda_.data(),
                    c.char_handle
                );
            }
        }
    }
}
void GATTClient::onRegForNotify(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    if(param->reg_for_notify.status != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to register for notification, status 0x%x", param->reg_for_notify.status);
        return;
    }

    // Récupérer le CCCD pour ce handle
    uint16_t handle = param->reg_for_notify.handle;  // c'est le handle de la char pour laquelle on veut la notif
    uint16_t cccd_handle = 0;

    uint16_t desc_count = 0;
    esp_err_t ret = esp_ble_gattc_get_attr_count(
        gattc_if,
        conn_id_,
        ESP_GATT_DB_DESCRIPTOR,
        0,                  // start_handle de service
        0xFFFF,             // end_handle de service (ou de ton service)
        handle,
        &desc_count
    );

    if(ret != ESP_GATT_OK || desc_count == 0){
        ESP_LOGW(GATTC_CLIENT_TAG(this->app_id_), "No descriptors found for handle 0x%x", handle);
        return;
    }

    std::vector<esp_gattc_descr_elem_t> descr(desc_count);
    ret = esp_ble_gattc_get_descr_by_char_handle(
        gattc_if,
        conn_id_,
        handle,
        notify_descr_uuid_,  // ton UUID 0x2902
        descr.data(),
        &desc_count
    );

    if(ret != ESP_GATT_OK || desc_count == 0){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to get CCCD for handle 0x%x", handle);
        return;
    }

    cccd_handle = descr[0].handle;

    // ✅ Écriture pour activer la notification
    uint16_t notify_en = 1;
    ret = esp_ble_gattc_write_char_descr(
        gattc_if,
        conn_id_,
        cccd_handle,
        sizeof(notify_en),
        (uint8_t*)&notify_en,
        ESP_GATT_WRITE_TYPE_RSP,
        ESP_GATT_AUTH_REQ_NONE
    );

    if(ret != ESP_GATT_OK){
        ESP_LOGE(GATTC_CLIENT_TAG(this->app_id_), "Failed to write CCCD for handle 0x%x, status 0x%x", cccd_handle, ret);
    } else {
        ESP_LOGI(GATTC_CLIENT_TAG(this->app_id_), "CCCD write sent for handle 0x%x", cccd_handle);
    }
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
void GATTClient::onDisconnect(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param){
    this->conn_device_ = 0;
}


// onNotify, onWriteDescr and onReadChar are pure virtual functions that will be implemented by derived classes to handle specific logic for notifications, descriptor writes and characteristic reads respectively.



