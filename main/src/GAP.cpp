#include "GAP.h"

GAP::GAP(std::string remote_device_name) : adv_config_done_(0), is_scanning_(0), is_advertising_(0) {
    adv_params_ = {
        .adv_int_min        = 0x20,
        .adv_int_max        = 0x40,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    remote_device_name_ = remote_device_name;
}


void GAP::gap_callback_bridge(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    GAP::getInstance().operator()(event, param);
}

void GAP::operator()(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    switch(event){
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:   onAdvDataSetCmpl(event, param); break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT: onScanRspDataSetCmpl(event, param); break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:      onAdvStartCmpl(event, param); break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:       onAdvStopCmpl(event, param); break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:      onUpdateConnParams(event, param); break;
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:      onScanStopCmpl(event, param); break;
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: onScanParamSetCmpl(event, param); break;
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:     onScanStartCmpl(event, param); break;
        case ESP_GAP_BLE_SCAN_RESULT_EVT:             onScanResult(event, param); break;
        default:
            ESP_LOGI(GAP_TAG, "Unhandled event: %x", event);
            break;
    }
}

void GAP::onAdvDataSetCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGI(GAP_TAG, "Adv Data set");
    adv_config_done_ &= (~(1 << 1));
    if (this->adv_config_done_ == 0 && !this->is_advertising_){
        this->is_advertising_ = 1;
        esp_ble_gap_start_advertising(&adv_params_);
    }
}
void GAP::onScanRspDataSetCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGI(GAP_TAG, "Scan Response Data set");
    this->adv_config_done_ &= (~(1 << 1));
    if (this->adv_config_done_ == 0 && !this->is_advertising_){
        this->is_advertising_ = 1;
        esp_ble_gap_start_advertising(&adv_params_);
    }
}
void GAP::onAdvStartCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if(param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) ESP_LOGE(GAP_TAG, "Adv start failed");
    else ESP_LOGI(GAP_TAG, "Adv started successfully");
}
void GAP::onAdvStopCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) ESP_LOGE(GAP_TAG, "Adv stop failed");
    else ESP_LOGI(GAP_TAG, "Adv stopped successfully");
    this->is_advertising_ = 0;
}
void GAP::onUpdateConnParams(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGI(GAP_TAG, "Conn params update status: %d", param->update_conn_params.status);
}
void GAP::onScanStopCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) ESP_LOGE(GAP_TAG, "Scan stop failed");
    else ESP_LOGI(GAP_TAG, "Scan stopped successfully");
    this->is_scanning_ = 0;
    /*if (!this->is_scanning_){
        esp_ble_gap_start_scanning(120);
        this->is_scanning_ = 1;
    }*/
}
void GAP::onScanParamSetCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGI(GAP_TAG, "Scan params set, starting scan...");
    if (!this->is_scanning_){
        esp_ble_gap_start_scanning(120);
        this->is_scanning_ = 1;
    }
}
void GAP::onScanStartCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) ESP_LOGE(GAP_TAG, "Scan start failed");
    else ESP_LOGI(GAP_TAG, "Scan started successfully");
}
void GAP::onScanResult(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    for(auto observer : observers_) {
        observer->Update(param);
     }  
}
void GAP::onSearchInqCmpl(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    ESP_LOGI(GAP_TAG, "Search Inquiry Complete, restarting scan");
    if (!this->is_scanning_){
        esp_ble_gap_start_scanning(120);
        this->is_scanning_ = 1;
    }
}
// --- Registration ---

void GAP::Update() {
    this->startAdvertising();
    /*if (!this->is_scanning_){
        esp_ble_gap_start_scanning(120);
        this->is_scanning_ = 1;
    }*/
}

void GAP::Attach(IObserver<esp_ble_gap_cb_param_t *> *observer) {
    this->observers_.push_back(observer);
}
void GAP::Detach(IObserver<esp_ble_gap_cb_param_t *> *observer) {
    this->observers_.remove(observer);
}
void GAP::Notify() {
    /*for(auto observer : observers_) {
        observer->Update();
    }*/
}

void GAP::startScanning(uint8_t duration_sec) {
    if (!this->is_scanning_){
        esp_ble_gap_start_scanning(duration_sec);
        this->is_scanning_ = 1;
    }
}
void GAP::stopScanning(){
    esp_ble_gap_stop_scanning();
    this->is_scanning_ = 0;
}

void GAP::startAdvertising(){
    if (this->adv_config_done_ == 0 && !this->is_advertising_){
        this->is_advertising_ = 1;
        esp_ble_gap_start_advertising(&adv_params_);
    }
}

void GAP::stopAdvertising(){
    esp_ble_gap_stop_advertising();
    this->is_advertising_ = 0;
}