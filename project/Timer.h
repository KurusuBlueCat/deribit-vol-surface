#pragma once

#include <map>
#include <string>
#include <chrono>
#include <iostream>

namespace timer{

class TimingContext {
public:
    std::map<std::string, double> timings;
};

//saves in milliseconds
class Timer {
public:
    TimingContext& ctx;
    std::string name;
    std::clock_t start;

    Timer(TimingContext& ctx, std::string name)
        : ctx(ctx),
        name(name),
        start(std::clock()) 
    {}

    ~Timer() {
        std::cout << static_cast<double>(start);
        ctx.timings[name] = static_cast<double>(std::clock() - start) * 1000 / static_cast<double>(CLOCKS_PER_SEC);
    }
};

} //end namespace timer