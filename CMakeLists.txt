# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
#set(EXCLUDE_COMPONENTS 
#    lwip 
#    esp_wifi 
#    esp_mesh 
#    esp_bridge 
#    esp_http_client 
#    esp_http_server 
#    esp_https_ota 
#    esp_https_server 
#    mqtt 
#    tcp_transport
#    wifi_provisioning
#)

set(COMPONENTS_EXCLUDE lwip esp_wifi esp_mesh esp_netif)    
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(BLE-ESPC6)
