#include <iostream>
#include <unordered_map>

#include "CsvFeeder.h"
#include "Msg.h"
#include "VolSurfBuilder.h"
#include "CubicSmile.h"


int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];

    VolSurfBuilder<CubicSmile> volBuilder;
    auto feeder_listener = [&volBuilder] (const Msg& msg) {
        if (msg.isSet) {
            volBuilder.Process(msg);
        }
    };

    auto timer_listener = [&volBuilder] (uint64_t now_ms) {
        // print information of vol builder - you need to implement the PrintInfo() function yourself
        volBuilder.PrintInfo();
    };

    const auto interval = std::chrono::minutes(60);  // we call timer_listener at 1 minute interval
    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);

    while (csv_feeder.Step()) {
    }
    return 0;
}