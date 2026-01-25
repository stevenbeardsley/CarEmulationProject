#ifndef ECM_PROCESS_H
#define ECM_PROCESS_H

#include <iostream>
#include <thread>
#include <chrono>

namespace ecm
{

class Process
{
public:
    Process() = default;
    void run();
};

}

#endif 