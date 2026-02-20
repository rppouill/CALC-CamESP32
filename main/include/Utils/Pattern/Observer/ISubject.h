#ifndef SUBJEC_ISUBJECT_H
#define SUBJEC_ISUBJECT_H

template <typename T>
class ISubject {
    public:
        virtual ~ISubject(){};
        virtual void Attach(T *observer) = 0;
        virtual void Detach(T *observer) = 0;
        virtual void Notify() = 0;
};

#endif //SUBJEC_ISUBJECT_H