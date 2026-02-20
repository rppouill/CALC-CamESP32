idf_component_register(
    SRCS "BLE-ESPC6.cpp" 
         "src/Server/GATTSImageBroadcaster.cpp"
         "src/Server/GATTCharacteristic.cpp"
         "src/Server/GATTCharacteristics.cpp"
         "src/GAP.cpp"
         "src/Client/GATTClient.cpp"
         "src/Client/GATTCImageReceiver.cpp"
         "src/Imager/BLUART.cpp"
         
    INCLUDE_DIRS "." 
                 "./include"
                 "./include/Server"
                 "./include/Client"
                 "./include/Imager"
                 "./include/Utils")

target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++23)                    