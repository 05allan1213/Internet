#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <cassert>
#include <functional>
#include <pthread.h>
#include <unistd.h>

namespace MyThread
{
    typedef std::function<void *(void *)> func_t;
    const int num = 1024;

    class Thread
    {
    private:
        // 在类内部创建线程，若希望线程执行对应的方法，需要将方法设为 static
        static void *thread_func(void *args) // 类内成员，有隐藏参数
        {
            Thread *_this = static_cast<Thread *>(args);
            return _this->callback();
        }

    public:
        Thread()
        {
            char namebuffer[num];
            snprintf(namebuffer, sizeof(namebuffer), "thread-%d", threadnum++);
            _name = namebuffer;
        }

        void start(func_t func, void *args = nullptr)
        {
            _func = func;
            _args = args;
            int n = pthread_create(&_tid, nullptr, thread_func, this);
            assert(n == 0);
            (void)n;
        }

        void join()
        {
            int n = pthread_join(_tid, nullptr);
            assert(n == 0);
            (void)n;
        }

        void *callback()
        {
            return _func(_args);
        }

        std::string threadname()
        {
            return _name;
        }

        ~Thread()
        {
            // do nothing
        }

    private:
        std::string _name;
        pthread_t _tid;
        func_t _func;
        void *_args;

        static int threadnum;
    };
    int Thread::threadnum = 1;
}
