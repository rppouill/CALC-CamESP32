// C++ includes
#include <vector>
#include <cstdio>
#include <string>

// ESP-IDF includes
    // FreeRTOS includes
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

    // NVS includes
#include <nvs.h>
#include <nvs_flash.h>

    // ESP includes
#include <esp_log.h>
#include <esp_system.h>
    
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

// Custom includes
#include "GATTService.h"
#include "GATTSImageBroadcaster.h"
#include "GATTCManager.h"
#include "GAP.h"
#include "GATTCImageReceiver.h"
#include "BLUART.h"
#include "DoubleBuffer.h"
#include "DataBlock.h"
#include "DataBase.h"

#define COEX_TAG "COEX"
#define IMAGE_SIZE 1024
#define BLE_DEBUG 0
#if BLE_DEBUG
    #include "Image_1.h"

    static void debug_task(void* pvParameters) {
        auto& profiles = *static_cast<std::vector<std::shared_ptr<GATTSImageBroadcaster>>*>(pvParameters);
        while(1){
            if(profiles[0]->getConnDevice() == 0){
                ESP_LOGW(COEX_TAG, "No device connected to send image.");
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            } else {
                profiles[0]->sendImage(IMAGE_SIZE, profiles[0]->getId());
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
    }
#endif

//static std::shared_ptr<DataBlock<std::vector<uint8_t>>> img_buffer = std::make_shared<DataBlock<std::vector<uint8_t>>>(IMAGE_SIZE, 10);
static std::string device_name;
static DataBase& database = DataBase::getInstance();
//static std::shared_ptr<DoubleBuffer<uint8_t>> img_buffer = std::make_shared<DoubleBuffer<uint8_t>>(IMAGE_SIZE);
static void bluetooth_task(void* pvParameter){
    auto& profiles = *static_cast<std::vector<std::shared_ptr<GATTSImageBroadcaster<DataBlock<std::vector<uint8_t>>>>>*>(pvParameter);
    auto& bluart = BLUART::getInstance(database.get_data_block(device_name));

    while(1){
        if(profiles[0]->getConnDevice() > 0){
            if(xSemaphoreTake(bluart.getSyncSemaphore(), portMAX_DELAY) == pdTRUE){
                profiles[0]->sendImage(IMAGE_SIZE, profiles[0]->getId());
                database.get_data_block(device_name)->next_read();
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
extern "C"
void app_main(void)
{
    esp_err_t ret;

    // Initialize NVS.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(COEX_TAG, "NVS initialized successfully");

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(COEX_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(COEX_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&bluedroid_cfg);
    if (ret) {
        ESP_LOGE(COEX_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(COEX_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    /*esp_power_level_t power_level = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
    ESP_LOGI(COEX_TAG, "power level before power_set: %d", power_level);
    ret =  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_N15);
    if(ret){
        ESP_LOGE(COEX_TAG, "set tx power failed, error code = %x", ret);
        return;
    }    
    power_level = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
    ESP_LOGI(COEX_TAG, "power level after power_set: %d", power_level);*/

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(517);
        if (local_mtu_ret) {
            ESP_LOGE(COEX_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
        }
    
    char dev_name[12];
    auto addr = esp_bt_dev_get_address();
    std::snprintf(dev_name, sizeof(dev_name), "CALC-%02X%02X%02X", addr[3], addr[4], addr[5]);
    device_name = std::string(dev_name);
    ESP_LOGI(COEX_TAG, "Device name: %s", device_name.c_str());
    auto& gattc_manager = GATTCManager<GATTCImageReceiver>::getInstance("CALC");

    database.create_new_block(device_name, IMAGE_SIZE, 10);
    database.get_data_block(device_name)->push_back(std::vector<uint8_t>(IMAGE_SIZE, 255)); // Initialize the first block with zeros

    // Initialize GAP (Scan & Adv)
    auto& gap_instance = GAP::getInstance(device_name);
    gap_instance.Attach(&gattc_manager); // Attach GATTCManager as an observer to GAP to receive scan results
    gattc_manager.Attach(&gap_instance); // Attach GAP as an observer to GATTCManager to receive connection status updates

    ret = esp_ble_gattc_register_callback(GATTCManager<GATTCImageReceiver>::gattc_callback_bridge);
    if(ret){
        ESP_LOGE(COEX_TAG, "gattc register error, error code = %x", ret); return;
        return;
    }

    ret = esp_ble_gap_register_callback(GAP::gap_callback_bridge);
    if (ret){
        ESP_LOGE(COEX_TAG, "gap register error, error code = %x", ret);
        return;
    }
    gap_instance.startScanning(120);
    
    // Initialize GATTS Service and Profiles
    auto imageBR = std::make_shared<GATTSImageBroadcaster<DataBlock<std::vector<uint8_t>>>>(device_name);
    imageBR->Attach(&gap_instance);
    static std::vector<std::shared_ptr<GATTSImageBroadcaster<DataBlock<std::vector<uint8_t>>>>> profiles = {imageBR};
    //static GATTService gatt_service(profiles);
    auto& gatt_service = GATTService<DataBlock<std::vector<uint8_t>>>::getInstance(profiles);
    ret = esp_ble_gatts_register_callback(GATTService<DataBlock<std::vector<uint8_t>>>::gatts_callback_bridge);
    if(ret){
        ESP_LOGE(COEX_TAG, "gatts register error, error code = %x", ret);
        return;
    }
    for(const auto& profile : profiles) {
        ret = esp_ble_gatts_app_register(profile->getId());
        if (ret) {
            ESP_LOGE(COEX_TAG, "gatts app register error, error code = %x", ret);
            return;
        }
    }

    gap_instance.startAdvertising();


    // Initialize GATTC (Client)
    /*uint16_t remote_service_uuid16 = 0x00FF;
    uint16_t notify_descr_uuid = 0xFF01;
    static auto gattc_manager = std::make_shared<GATTCManager>(remote_service_uuid16, notify_descr_uuid);
    ret = esp_ble_gattc_register_callback(GATTCManager::gattc_callback_bridge);
    if(ret){
        ESP_LOGE(COEX_TAG, "gattc register error, error code = %x", ret);
        return;
    }
    for(uint8_t i = 0; i < gattc_manager->getMaxClients(); i++){
        ret = esp_ble_gattc_app_register(i);
        if (ret) {
            ESP_LOGE(COEX_TAG, "gattc app register error, error code = %x", ret);
            return;
        }
    }*/

    /*ret = esp_ble_gattc_register_callback(GATTCManager::gattc_callback_bridge);
    if(ret){
        ESP_LOGE(COEX_TAG, "gattc register error, error code = %x", ret);
        return;
    }*/
    // Instanciate Double Buffer
    //DoubleBuffer<uint8_t> double_buffer(IMAGE_SIZE);

    
    

    ESP_LOGI(COEX_TAG, "All systems GO. GATTS, GATTC and GAP initialized.");


    

    ESP_LOGI(COEX_TAG, "Bluetooth initialized successfully");
    
    #if BLE_DEBUG
        std::memcpy(img_buffer.get(), img, IMAGE_SIZE);
        ESP_LOGI(COEX_TAG, "Create debug loop task");
        xTaskCreate(debug_task, "debug_task", 2048, (void*)&profiles, 12, NULL);
    #endif

    auto& bluart = BLUART::getInstance(database.get_data_block(device_name));
    xTaskCreate(BLUART::task_wrapper, "bluart_task", 2048, NULL, 10, NULL);
    xTaskCreate(bluetooth_task, "bluetooth_task", 2048, (void*)&profiles, 9, NULL);

    return;
}



