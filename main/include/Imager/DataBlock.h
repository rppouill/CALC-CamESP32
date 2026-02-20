#ifndef DATABLOCK_H
#define DATABLOCK_H

#include "Utils/CircularBuffer.hpp"

#include <unordered_map>
#include <string>

template <typename T>
class DataBlock {
    private:
        CircularBuffer<T> data_buffer_; // Circular buffer to hold the data blocks
        size_t block_size_; // Size of each block in bytes
        size_t max_elements_; // Maximum number of elements in the circular buffer

        uint8_t read_block;
        uint8_t write_block; // Basically it's the head of the circular buffer

    public:
        DataBlock(size_t block_size, size_t max_elements) : data_buffer_(max_elements), block_size_(block_size), max_elements_(max_elements) {
            this->write_block = this->data_buffer_.getHead();
            this->read_block = this->data_buffer_.getTail();
        }

        void next_read(){
            read_block = (read_block + 1) % max_elements_;
        }

        void push_back(const T& data) {
            this->data_buffer_.push_back(data);
            this->write_block = this->data_buffer_.getHead();
        }

        T& operator[](size_t index) {
            return data_buffer_[index];
        }

        CircularBuffer<T>& get_buffer() {
            return data_buffer_;
        }

        uint8_t get_read_block() const {
            return read_block;
        }
        uint8_t get_write_block() const {
            return write_block;
        }
    
};




#endif // DATABLOCK_H