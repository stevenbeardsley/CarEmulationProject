#include "Process.h"
#include <iostream>
#include <thread>

namespace tcm
{

void Process::run()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Keep the container open and script running forever
    }
}

}
