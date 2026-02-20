#ifndef CIRCULARBUFFER_H
#define CIRCULARBUFFER_H
#include <vector>

#include <numeric>
template <typename T>
class CircularBuffer{
    public:
        CircularBuffer(int size) : size(size), buffer(size), head(0), tail(0), full(false) {}
        
        ~CircularBuffer(){
            buffer.clear();
        }

        void push(T value){
            buffer.insert(buffer.begin(), value);
            buffer.pop_back();

            this->next_block();
        }


        void push_back(T value){
            buffer.push_back(value);
            buffer.erase(buffer.begin());            
            this->next_block();
        }

        const std::vector<T>& getBuffer() const {
            return buffer;
        }

        T& operator[](size_t index) {
            return buffer[index];
        }

        T pop(){
            if(isEmpty()){
                return 0;
            }
            T value = buffer[tail];
            full = false;
            tail = (tail + 1) % size;
            return value;
        }
        bool isEmpty(){
            return (!full && (head == tail));
        }
        bool isFull(){
            return full;
        }
        int getSize(){
            return size;
        }
        

        void next_block(){
            head = (head + 1) % size;
            tail = full ? (tail + 1) % size : tail;
            full = (head == tail);
        }
        int getHead(){
            return head;
        }
        int getTail(){
            return tail;
        }
    private:
        int size;
        std::vector<T> buffer;
        int head;
        int tail;
        bool full;
};

#endif // CIRCULARBUFFER_H