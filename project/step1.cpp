#include <iostream>

#include "CsvFeeder.h"
#include "Msg.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];

    int n=0;

    auto feeder_listener = [&] (const Msg& msg) {
        ++n;
        //this only print information if msg is a snap
        if (msg.isSnap) //comment this line to print everything 
        {
            std::cout << msg.timestamp << ", isSnap = " << msg.isSnap << ", numUpdates = " << msg.Updates.size() << std::endl;
        }
    };

    auto timer_listener = [&] (uint64_t now_ms) {
        std::cout << "timer_listener called: " << now_ms << std::endl;
        std::cout << "Line read: " << n << std::endl;
    };

    const auto interval = std::chrono::minutes(1);  // we call timer_listener at 1 minute interval
    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);

    while (csv_feeder.Step()) {
    }
    return 0;
}