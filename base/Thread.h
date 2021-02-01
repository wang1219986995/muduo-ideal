
/******************************************************
*   Copyright (C)2019 All rights reserved.
*
*   Author        : owb
*   Email         : 2478644416@qq.com
*   File Name     : Thread.h
*   Last Modified : 2020-12-11 14:59
*   Describe      :
*
*******************************************************/

#ifndef  _IDEAL_BASE_THREAD_H
#define  _IDEAL_BASE_THREAD_H

#include "NonCopyable.h"
#include "CountDownLatch.h"

#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace ideal {


class Thread : public NonCopyable {
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string& name = "");
    ~Thread();

    void start();
    void join();

    bool started() const {
        return _started;
    }

    pid_t tid() const {
        return _tid;
    }

    const std::string& name() const {
        return _name;
    }

    static int numCreated() {
         return _numCreated;
    }

private:
    void setDefaultName();

private:
    bool _started;
    bool _joined;
    pid_t _tid;
    std::thread _thread;
    ThreadFunc _func;
    std::string _name;
    CountDownLatch _latch;

    static std::atomic<int> _numCreated;
};

}


#endif // _IDEAL_BASE_THREAD_H


