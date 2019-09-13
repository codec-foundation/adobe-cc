#ifndef FREELIST_HPP
#define FREELIST_HPP

#include <functional>
#include <memory>

// thread-safe freelist
template<class T>
class FreeList
{
public:
    typedef std::unique_ptr<T, std::function<void(T*)>> PooledT;

    FreeList(std::function<std::unique_ptr<T> ()> factory) : factory_(factory) {}

    PooledT allocate()
    {
        std::lock_guard<std::mutex> guard(mutex_);
        if (freeList_.empty()) {
            return PooledT(
                factory_().release(),
                [&](T* repool) { free(repool); }
            );
        }
        else {
            auto t = std::move(freeList_.back());
            freeList_.pop_back();
            return PooledT(
                t.release(),
                [&](T* repool) { free(repool); }
            );
        }
    }

private:
    void free(T* t)
    {
        std::lock_guard<std::mutex> guard(mutex_);
        freeList_.push_back(std::unique_ptr<T>(t));
    }

    std::mutex mutex_;
    std::vector<std::unique_ptr<T>> freeList_;
    std::function<std::unique_ptr<T> ()> factory_;
};

#endif