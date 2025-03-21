#pragma once

#include <pthread.h>

class Mutex
{
public:
    Mutex(pthread_mutex_t *lock) : _lock(lock)
    {
    }
    void Lock()
    {
        if (_lock)
            pthread_mutex_lock(_lock);
    }
    void Unlock()
    {
        if (_lock)
            pthread_mutex_unlock(_lock);
    }
    ~Mutex()
    {
    }

    pthread_mutex_t *_lock;
};

class LockGuard
{
public:
    LockGuard(pthread_mutex_t *lock) : _mutex(lock)
    {
        _mutex.Lock();
    }
    ~LockGuard()
    {
        _mutex.Unlock();
    }

private:
    Mutex _mutex;
};