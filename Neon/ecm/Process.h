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