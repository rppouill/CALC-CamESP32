#ifndef DATABASE_H
#define DATABASE_H

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include "Pattern/Singleton/Singleton.h"
#include "DataBlock.h"

class DataBase : public Singleton<DataBase> {
    friend class Singleton<DataBase>;
    private:
        std::unordered_map<std::string, 
                std::shared_ptr<DataBlock<std::vector<uint8_t>>>> data_blocks_;
        
    public:
        DataBase() = default;

        void create_new_block(const std::string& base_name, size_t block_size, size_t max_elements) {
            if(!this->data_blocks_.contains(base_name)){
                data_blocks_.emplace(base_name, std::make_shared<DataBlock<std::vector<uint8_t>>>(block_size, max_elements));
                //data_blocks_[base_name] = std::make_shared<DataBlock<std::vector<uint8_t>>>(block_size, max_elements);
                data_blocks_[base_name]->push_back(std::vector<uint8_t>(block_size, 64));
            }
        }
        
        std::unordered_map<std::string, std::shared_ptr<DataBlock<std::vector<uint8_t>>>>& get_data_block() {
            return data_blocks_;
        }
        
        std::shared_ptr<DataBlock<std::vector<uint8_t>>> operator[](const std::string& base_name) {
            return data_blocks_.at(base_name);
        }
        /*std::shared_ptr<DataBlock<std::vector<uint8_t>>> get_data_block(const std::string& base_name) {
            return data_blocks_.at(base_name);
        }*/
};

#endif // DATABASE_H