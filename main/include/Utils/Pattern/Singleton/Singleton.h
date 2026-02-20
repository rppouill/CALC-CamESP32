#include <memory>
#include <vector>

#ifndef SINGLETON_H
#define SINGLETON_H

template <typename T>
class Singleton {
public:
    static T& getInstance() {
        if (!_instance) {
            _instance.reset(new T()); // Construction directe avec new
        }
        return *_instance;
    }
    template <typename... Args>
    static T& getInstance(Args&&... args){
        if(!_instance){
            _instance.reset(new T(std::forward<Args>(args)...));
        }
        return *_instance;
    }

    void attach(const T& node) {
        nodes.push_back(node);
    }
    void detach() { /* Implémentation */ }
    void notify() { /* Implémentation */ }

protected:
    Singleton() = default;
    virtual ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

private:
    static std::unique_ptr<T> _instance;
    std::vector<T> nodes;
};

template <typename T>
std::unique_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H