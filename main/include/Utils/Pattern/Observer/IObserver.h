#ifndef OBSERVER_IOBSERVER_H
#define OBSERVER_IOBSERVER_H

template <typename... Args>
class IObserver {
    public:
        virtual ~IObserver(){};
        virtual void Update(const Args &... args) = 0;
};

#endif //OBSERVER_IOBSERVER_H