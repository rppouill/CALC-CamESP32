#ifndef DOUBLE_BUFFER_H
#define DOUBLE_BUFFER_H

#include <atomic>
#include <memory>
#include <cstdint>

template<typename T>
class DoubleBuffer {
    private:
        std::shared_ptr<T[]> buffers[2];
        std::atomic<uint8_t> front_idx;
        size_t buffer_size;

    public:
        DoubleBuffer(size_t size) : buffer_size(size) {
            buffers[0] = std::make_shared<T[]>(size);
            buffers[1] = std::make_shared<T[]>(size);
            front_idx.store(0, std::memory_order_relaxed);
        }

        void swap(){
            uint8_t next_idx = 1 - front_idx.load(std::memory_order_relaxed);
            front_idx.store(next_idx, std::memory_order_release);
        }
        //const T& operator[](size_t idx) const{
        //    return buffers[front_idx.load(std::memory_order_acquire)][idx];
        //}
        
        T& operator[](size_t idx) {
            uint8_t back_idx = 1 - front_idx.load(std::memory_order_relaxed);
            return buffers[back_idx][idx];
        }
        
        T* get(){
            return buffers[1 - front_idx.load(std::memory_order_acquire)].get();
        }
        operator T*() {
            return get();
        }

        size_t size() const { 
            return buffer_size;
        }


        //void setBufferIdx(uint8_t idx, T value){
        //    buffers[front_idx][idx] = value;
        //}
};

#endif