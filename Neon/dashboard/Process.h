#ifndef DASHBOARD_PROCESS_H
#define DASHBOARD_PROCESS_H

#include <iostream>
#include <thread>
#include <chrono>

namespace dashboard
{

    class Process
    {
    public:
        Process() = default;
        void run();
    };

}

#endif