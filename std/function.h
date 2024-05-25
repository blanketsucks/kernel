#pragma once

#include <std/utility.h>

namespace std {

template<typename Signature>
class Function;

template<typename ReturnType, typename... Args>
class Function<ReturnType(Args...)> {
public:
    Function(nullptr_t) : callable(nullptr) {}

    template<typename Functor>
    Function(Functor functor) : callable(new CallableImpl<Functor>(functor)) {}

    Function(const Function& other) : callable(nullptr) {
        if (other.callable) {
            callable = other.callable->clone();
        }
    }

    Function(Function&& other) : callable(other.callable) { other.callable = nullptr; }

    ~Function() { 
        if (callable) {
            delete callable;
        }
    }

    Function& operator=(const Function& other) {
        if (this == &other) {
            return *this;
        }

        if (callable) {
            delete callable;
            callable = nullptr;
        }

        if (other.callable) {
            callable = other.callable->clone();
        }
    }

    Function& operator=(Function&& other) {
        if (this != &other) {
            if (callable) {
                delete callable;
            }

            callable = other.callable;
            other.callable = nullptr;
        }

        return *this;
    }

    ReturnType operator()(Args... args) const {
        if (!callable) {
            return ReturnType();
        }

        return callable->invoke(std::forward<Args>(args)...);
    }

private:
    class CallableBase {
    public:
        virtual ReturnType invoke(Args...) const = 0;
        virtual CallableBase* clone() const = 0;

        virtual ~CallableBase() = default;
    };

    template<typename Functor>
    class CallableImpl : public CallableBase {
    public:
        CallableImpl(const Functor& functor) : functor(functor) {}

        ReturnType invoke(Args... args) const override {
            return functor(std::forward<Args>(args)...);
        }

        CallableBase* clone() const override {
            return new CallableImpl<Functor>(functor);
        }

    private:
        Functor functor;
    };

    CallableBase* callable;
};

}

using std::Function;