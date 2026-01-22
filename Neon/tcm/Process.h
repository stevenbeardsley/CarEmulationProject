#include <iostream>
#include <thread>
#include <chrono>

namespace tcm
{

    class Process
    {
    public:
        Process() = default;
        void run();
    };

}