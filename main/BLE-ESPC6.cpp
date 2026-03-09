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
#include "GATTSManager.h"
#include "GATTCManager.h"
#include "GAP.h"
#include "GATTCImageReceiver.h"
#include "GATTSImageBroadcaster.h"
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
static void uart2bluetooth(void* pvParameter){
    auto service = GATTSManager<GATTSImageBroadcaster, DataBlock<std::vector<uint8_t>>>::getInstance()[0x00FF];
    auto& bluart = BLUART::getInstance(database[device_name]);

    auto bdService = GATTSManager<GATTSImageBroadcaster, DataBlock<std::vector<uint8_t>>>::getInstance()[0x00FA];
    while(1){
        if(service->getConnDevice() != 0 && (bdService->getAskDevice().empty() || bdService->getAskDevice() == device_name)) {
            xSemaphoreTake(database[device_name]->getSyncSemaphore(), portMAX_DELAY);
            UBaseType_t count = uxSemaphoreGetCount(database[device_name]->getSyncSemaphore());
            //ESP_LOGI("SEM", "Count = %u", count);
            service->broadcast(database[device_name]->operator[](database[device_name]->get_read_block()).data(),
                                IMAGE_SIZE / 2, false, 4, "CALC-36328C");
            service->broadcast(database[device_name]->operator[](database[device_name]->get_read_block()).data() + IMAGE_SIZE / 2,
                                IMAGE_SIZE / 2, false, 6, "CALC-36328C");
            database[device_name]->next_read();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void bluetooth2bluetooth(void *pvParameter){
    auto service = GATTSManager<GATTSImageBroadcaster, DataBlock<std::vector<uint8_t>>>::getInstance()[0x00FA];
    std::string ask_device;
    while(1){
        if(!service->getAskDevice().empty() && database.get_data_block().contains(service->getAskDevice())) {
            ask_device = service->getAskDevice();
            if(service->getConnDevice() != 0){
                auto data_block = database[ask_device];
                xSemaphoreTake(data_block->getSyncSemaphore(), portMAX_DELAY);
                UBaseType_t count = uxSemaphoreGetCount(data_block->getSyncSemaphore());
                ESP_LOGI("SEM", "Count = %u - [%d;%d] | ask_device: %s", count, data_block->get_read_block(), data_block->get_write_block(), ask_device.c_str());
                service->broadcast(data_block->operator[](data_block->get_read_block()).data(),
                                    IMAGE_SIZE / 2, false, 4, ask_device);
                service->broadcast(data_block->operator[](data_block->get_read_block()).data() + IMAGE_SIZE / 2,
                                    IMAGE_SIZE / 2, false, 6, ask_device);
                data_block->next_read();            
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

/*

static void bluetooth2bluetooth(void *pvParameter){
    auto service = GATTSManager<GATTSImageBroadcaster, DataBlock<std::vector<uint8_t>>>::getInstance()[0x00FA];
    std::string ask_device;
    while(1){
        if(!service->getAskDevice().empty() && database.get_data_block().contains(service->getAskDevice())) {
            ask_device = service->getAskDevice();
            if(service->getConnDevice() != 0){
                service->broadcast(database[ask_device]->operator[](database[ask_device]->get_read_block()).data(),
                                    IMAGE_SIZE / 2, false, 4, ask_device);
                service->broadcast(database[ask_device]->operator[](database[ask_device]->get_read_block()).data() + IMAGE_SIZE / 2,
                                    IMAGE_SIZE / 2, false, 6, ask_device);
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}*/
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

    // Initialize GAP (Scan & Adv)
    esp_ble_adv_params_t adv_params = {
        .adv_int_min        = 0x20,
        .adv_int_max        = 0x40,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
    };
    auto& gap_instance = GAP::getInstance(device_name, adv_params);
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
    
    static uint8_t adv_service_uuid128[32] = {
        /* LSB <--------------------------------------------------------------------------------> MSB */
        //first uuid, 16bit, [12],[13] is the value
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
        // second uuid, 16bit, [28],[29] is the value
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFA, 0x00, 0x00, 0x00,
    };
    static esp_ble_adv_data_t adv_data = {
        .set_scan_rsp = false,
        .include_name = true,
        .include_txpower = true,
        //.min_interval = 0x0006,
        //.max_interval = 0x0010,
        .min_interval = 0x20,
        .max_interval = 0x40,
        .appearance = 0x00,
        .manufacturer_len = 0,
        .p_manufacturer_data =  NULL,
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = sizeof(adv_service_uuid128),
        .p_service_uuid = adv_service_uuid128,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };
    static esp_ble_adv_data_t scan_rsp_data = {
        .set_scan_rsp = true,
        .include_name = true,
        .include_txpower = true,
        .min_interval = 0x0006,
        .max_interval = 0x0010,
        .appearance = 0x00,
        .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
        .p_manufacturer_data =  NULL, //&test_manufacturer[0],
        .service_data_len = 0,
        .p_service_data = NULL,
        .service_uuid_len = sizeof(adv_service_uuid128),
        .p_service_uuid = adv_service_uuid128,
        .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
    };

    // Initialize GATTS Service and Profiles
    auto& GATTSManagerInstance = GATTSManager<GATTSImageBroadcaster, DataBlock<std::vector<uint8_t>>>::getInstance(device_name, adv_data, scan_rsp_data);
    
    std::unordered_map<uint16_t, std::vector<std::tuple<
        uint16_t,           // char uuid
        uint8_t,            // char properties
        uint16_t,           // char max length    
        uint8_t*            // char value buffer
    >>> services_maps = {
        {0x00FF, {
            {0xFF01, ESP_GATT_CHAR_PROP_BIT_WRITE, 11, nullptr},
            {0xFF02, ESP_GATT_CHAR_PROP_BIT_NOTIFY, IMAGE_SIZE / 2, database[device_name]->operator[](database[device_name]->get_read_block()).data()},
            {0xFF03, ESP_GATT_CHAR_PROP_BIT_NOTIFY, IMAGE_SIZE / 2, database[device_name]->operator[](database[device_name]->get_read_block()).data() + 512},
        }}, 
        {0x00FA, {
            {0xFA01, ESP_GATT_CHAR_PROP_BIT_WRITE, 11, nullptr},
            {0xFA02, ESP_GATT_CHAR_PROP_BIT_NOTIFY, IMAGE_SIZE / 2, database[device_name]->operator[](database[device_name]->get_read_block()).data()},
            {0xFA03, ESP_GATT_CHAR_PROP_BIT_NOTIFY, IMAGE_SIZE / 2, database[device_name]->operator[](database[device_name]->get_read_block()).data() + 512},
        }}
    };
    for(const auto& service_map : services_maps){
        GATTSManagerInstance.create_service(service_map.first, service_map.second);
    }
    ret = esp_ble_gatts_register_callback(GATTSManager<GATTSImageBroadcaster, DataBlock<std::vector<uint8_t>>>::gatts_callback_bridge);
    if(ret){
        ESP_LOGE(COEX_TAG, "gatts register error, error code = %x", ret);
        return;
    } else {
        ESP_LOGI(COEX_TAG, "GATTS callback registered successfully");
    }
    ret = GATTSManagerInstance.register_service();
    if(ret){
        ESP_LOGE(COEX_TAG, "Failed to register GATTS service, error code = %x", ret);
    } 
    


    
    
    ESP_LOGI(COEX_TAG, "All systems GO. GATTS, GATTC and GAP initialized.");
    ESP_LOGI(COEX_TAG, "Bluetooth initialized successfully");
    
    #if BLE_DEBUG
        std::memcpy(img_buffer.get(), img, IMAGE_SIZE);
        ESP_LOGI(COEX_TAG, "Create debug loop task");
        xTaskCreate(debug_task, "debug_task", 2048, (void*)&profiles, 12, NULL);
    #endif

    auto& bluart = BLUART::getInstance(database[device_name]);
    xTaskCreate(BLUART::task_wrapper, "bluart_task", 2048, NULL, 10, NULL);
    xTaskCreate(uart2bluetooth, "uart_to_bluetooth", 2048, NULL, 9, NULL);
    xTaskCreate(bluetooth2bluetooth, "bluetooth_to_bluetooth", 2048, NULL, 11, NULL);
    

    gap_instance.startAdvertising();
    gap_instance.startScanning(120);


    return;
}



