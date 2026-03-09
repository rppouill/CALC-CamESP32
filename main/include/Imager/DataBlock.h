#ifndef DATABLOCK_H
#define DATABLOCK_H

#include "Utils/CircularBuffer.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


#include <unordered_map>
#include <string>

template <typename T>
class DataBlock {
    private:
        CircularBuffer<T> data_buffer_; // Circular buffer to hold the data blocks
        size_t block_size_; // Size of each block in bytes
        size_t max_elements_; // Maximum number of elements in the circular buffer
        SemaphoreHandle_t syncSemaphore;
        


        uint8_t read_block;
        uint8_t write_block; // Basically it's the head of the circular buffer

    public:
        DataBlock(size_t block_size, size_t max_elements) : data_buffer_(max_elements), block_size_(block_size), max_elements_(max_elements), syncSemaphore(xSemaphoreCreateCounting(max_elements, 0)) {
            this->write_block = this->data_buffer_.getHead();
            this->read_block = this->data_buffer_.getTail();
        }
        SemaphoreHandle_t getSyncSemaphore() {
            return this->syncSemaphore;
        }
        
        void next_read(){
            //read_block = this->data_buffer_.getTail();
            //read_block = (read_block + 1) % max_elements_;
            read_block = max_elements_ - 1;
        }

        /*void next_write(){
            while((write_block + 1) % max_elements_ == read_block){
                vTaskDelay(10 / portTICK_PERIOD_MS);
            } write_block = (write_block + 1) % max_elements_;
        }*/

        void push_back(const T& data) {
            //this->next_write();
            this->data_buffer_.push_back(data);
            write_block = this->data_buffer_.getHead();
            //read_block = this->data_buffer_.getTail();
            xSemaphoreGive(this->syncSemaphore);
        }

        T& operator[](size_t index) {
            return data_buffer_[index];
        }

        CircularBuffer<T>& get_buffer() {
            return data_buffer_;
        }

        uint8_t get_read_block() {
            return read_block;
        }
        uint8_t get_write_block() const {
            return write_block;
        }
    
};




#endif // DATABLOCK_H