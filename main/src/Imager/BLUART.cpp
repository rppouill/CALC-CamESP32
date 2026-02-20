#include "BLUART.h"


BLUART::BLUART(std::shared_ptr<DataBlock<std::vector<uint8_t>>> data_block) : parity_flag(true), idx(0),  data_block_(data_block), syncSemaphore(xSemaphoreCreateBinary()) {
    // Configure UART parameters
    uart_config_t uart_config = {
        .baud_rate = BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    ESP_ERROR_CHECK(uart_driver_install(PORT_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(PORT_NUM, TXD, RXD, RTS, CTS));

    uart_set_rts(PORT_NUM, 0);
}


void BLUART::operator()(void *pvParameter)
{   
    uart_set_rts(PORT_NUM, 0);
    ESP_LOGI(BLUART_TAG, "UART event task started");
    std::vector<uint8_t> dtmp(BUF_SIZE, 0);
    while(1){
        if(gpio_get_level(RTS) == 0) parity_flag = true;
        if(xQueueReceive(uart_queue, (void*)&event, (TickType_t)portMAX_DELAY)){
            std::fill(dtmp.begin(), dtmp.end(), 0);
            switch(event.type){
                case UART_DATA: onData(dtmp); break;
                case UART_FIFO_OVF: onFifoOvf(); break;
                case UART_BUFFER_FULL: onBufferFull(); break;
                case UART_BREAK: onBreak(); break;
                case UART_PARITY_ERR: onParityErr(); break;
                case UART_FRAME_ERR: onFrameErr(); break;
                case UART_PATTERN_DET: onPatternDetect(dtmp); break;
                default: ESP_LOGI(BLUART_TAG, "uart event type: %d", event.type);
            }
        }
        //vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


void BLUART::task_wrapper(void *pvParameter) {
    BLUART::getInstance().operator()(pvParameter);
}

// State Machine Handlers

void BLUART::onData(std::vector<uint8_t>& dtmp) {
    static uint16_t pxl = 0x0000;
    static std::vector<uint8_t> img_buffer(BUF_SIZE, 127);
    uart_read_bytes(PORT_NUM, dtmp.data(), event.size, portMAX_DELAY);
    uart_set_rts(PORT_NUM, 0);
    for(auto i = 0; i < event.size; i++) {
        /*pxl = (dtmp[i] << 8) | dtmp[i + 1];
        //this->img_buffer_[(it - dtmp.begin()) / 2] = static_cast<uint8_t>((pxl & 0x1FFF) >> 4);
        //this->img_buffer_->operator[]((it - dtmp.begin()) / 2) = static_cast<uint8_t>((pxl & 0x1FFF) >> 4);
        if(pxl & 0x2000){
            ESP_LOGI(BLUART_TAG, "New Image detected: idx: %d, pxl: %04x, dtmp.size(): %d", idx, pxl, event.size);
            this->img_buffer_->swap();
            xSemaphoreGive(this->syncSemaphore);
            idx = 0;
        }
        this->img_buffer_->operator[](idx) = static_cast<uint8_t>((pxl & 0x1FFF) >> 4);
        idx++;*/

        if(parity_flag){
            pxl = dtmp[i] << 8; parity_flag = !parity_flag;
        } else {
            pxl |= dtmp[i]; parity_flag = !parity_flag;
            if(pxl & 0x2000){
                this->data_block_->push_back(img_buffer);
                xSemaphoreGive(this->syncSemaphore);
                idx = 0;
            } 
            img_buffer[idx++] = static_cast<uint8_t>((pxl & 0x1FFF) >> 4);
            
        }
    }
    uart_set_rts(PORT_NUM, 1);

}

/*
if(parity_flag){
    pxl = dtmp[i] << 8; parity_flag = !parity_flag;
} else {
    pxl |= dtmp[i]; parity_flag = !parity_flag;
    if(pxl & 0x2000){
        //ESP_LOGI(BLUART_TAG, "New Image detected: idx: %d, pxl: %04x, dtmp.size(): %d", idx, pxl, event.size);
        this->img_buffer_->next_block();
        xSemaphoreGive(this->syncSemaphore);
        idx = 0;
    }
    this->img_buffer_->operator[](idx++) = static_cast<uint8_t>((pxl & 0x1FFF) >> 4);
}

*/


void BLUART::onFifoOvf() {
    ESP_LOGW(BLUART_TAG, "FIFO overflow");
    uart_set_rts(PORT_NUM, 0);
}

void BLUART::onBufferFull() {
    ESP_LOGW(BLUART_TAG, "Ringbuffer full: %x", this->event.size);
    uart_set_rts(PORT_NUM, 0);
}

void BLUART::onBreak() {
    ESP_LOGW(BLUART_TAG, "RX break");
}

void BLUART::onParityErr() {
    ESP_LOGW(BLUART_TAG, "Parity error");
}

void BLUART::onFrameErr() {
    ESP_LOGW(BLUART_TAG, "Frame error");
}

void BLUART::onPatternDetect(std::vector<uint8_t>& dtmp) {
    size_t buffered_size = 0;
    uart_get_buffered_data_len(PORT_NUM, &buffered_size);
    auto pos = uart_pattern_pop_pos(PORT_NUM);
    ESP_LOGI(BLUART_TAG, "[PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
    if(pos == -1) {
        uart_flush_input(PORT_NUM);
    } else {
        uart_read_bytes(PORT_NUM, dtmp.data(), pos, 100 / portTICK_PERIOD_MS);
        std::vector<uint8_t> pat(PORT_NUM + 1, 0);
        uart_read_bytes(PORT_NUM, pat.data(), 3, 100 / portTICK_PERIOD_MS);
        ESP_LOGI(BLUART_TAG, "Read data: %s", dtmp.data());
        ESP_LOGI(BLUART_TAG, "Read pattern: %s", pat.data());
    }
}


SemaphoreHandle_t BLUART::getSyncSemaphore() {
    return this->syncSemaphore;
}