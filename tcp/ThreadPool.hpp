#pragma once

#include "Thread.hpp"
#include "LockGuard.hpp"
#include "Task.hpp"
#include "log.hpp"
#include <vector>
#include <queue>

using namespace MyThread;

const int MAX_SIZE = 5;

template <class T>
class ThreadPool;

template <class T>
class ThreadData
{
public:
    ThreadPool<T> *threadpool;
    std::string name;

public:
    ThreadData(ThreadPool<T> *tp, std::string n) : threadpool(tp), name(n)
    {
    }
};

template <class T>
class ThreadPool
{
private:
    static void *handlerTask(void *args)
    {
        ThreadData<T> *thread_data = (ThreadData<T> *)args;
        while (true)
        {
            T task;
            {
                LockGuard lockguard(thread_data->threadpool->mutex());
                while (thread_data->threadpool->isempty())
                {
                    thread_data->threadpool->threadWait();
                }
                task = thread_data->threadpool->pop();
            }
            task();
        }
        delete thread_data;
        return nullptr;
    }

    ThreadPool(const int &num = MAX_SIZE)
        : _num(num)
    {
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond, nullptr);
        for (int i = 0; i < _num; ++i)
        {
            _threads.push_back(new Thread());
        }
    }

    void operator=(const ThreadPool &) = delete;

    ThreadPool(const ThreadPool &) = delete;

public:
    void lockQueue()
    {
        pthread_mutex_lock(&_mutex);
    }

    void unlockQueue()
    {
        pthread_mutex_unlock(&_mutex);
    }

    bool isempty()
    {
        return _task_queue.empty();
    }

    void threadWait()
    {
        pthread_cond_wait(&_cond, &_mutex);
    }

    T pop()
    {
        T task = _task_queue.front();
        _task_queue.pop();
        return task;
    }

    pthread_mutex_t *mutex()
    {
        return &_mutex;
    }

public:
    void run()
    {
        for (const auto &t : _threads)
        {
            ThreadData<T> *thread_data = new ThreadData<T>(this, t->threadname());
            t->start(handlerTask, thread_data);
            // std::cout << t->threadname() << " start..." << std::endl;
            logMessage(DEBUG, "%s start...", t->threadname().c_str());
        }
    }

    void Push(const T &in)
    {
        LockGuard lockguard(&_mutex);
        _task_queue.push(in);
        pthread_cond_signal(&_cond);
    }

    ~ThreadPool()
    {
        pthread_mutex_destroy(&_mutex);
        pthread_cond_destroy(&_cond);
        for (const auto &t : _threads)
            delete t;
    }

    static ThreadPool<T> *getInstance()
    {
        if (nullptr == _instance)
        {
            _single_mutex.lock();
            if (nullptr == _instance)
            {
                _instance = new ThreadPool<T>();
            }
            _single_mutex.unlock();
        }
        return _instance;
    }

private:
    int _num;
    std::vector<Thread *> _threads;
    std::queue<T> _task_queue;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;

    static ThreadPool<T> *_instance;
    static std::mutex _single_mutex;
};

template <class T>
ThreadPool<T> *ThreadPool<T>::_instance = nullptr;

template <class T>
std::mutex ThreadPool<T>::_single_mutex;