#include <iostream>
#include <iomanip> // for std::put_time and std::setw

#include "CsvFeeder.h"
#include "Msg.h"
#include "VolSurfBuilder.h"
#include "CubicSmile.h"
#include <Eigen/Core>
#include <LBFGSB.h>
#include <sstream>

template class VolSurfBuilder<CubicSmile>;

std::string convertUnixMSToISO8601(uint64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp / 1000);
    std::tm* dateTime = std::gmtime(&time);

    std::ostringstream oss;
    oss << std::put_time(dateTime, "%Y-%m-%dT%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << timestamp % 1000 << "Z";

    return oss.str();
}

std::string convertDateFormat(const std::string& date) {
    std::tm dateTime = {};
    std::istringstream iss(date);

    iss >> std::get_time(&dateTime, "%Y-%m-%d");

    if (iss.fail()) {
        return "Invalid date format";
    }

    std::ostringstream oss;
    oss << std::put_time(&dateTime, "%d-%b-%Y");

    return oss.str();
}

void csvLineReport(const FitSmileResult& smileResult, std::ofstream& outputFile){

    // std::string line = "TIME, EXPIRY, FUT_PRICE, ATM, BF25, RR25, BF10, RR10\n";
    std::string dateTime = convertUnixMSToISO8601(smileResult.LastUpdateTimeStamp);
    std::string line = dateTime + ",";

    std::stringstream ss;    
    ss << smileResult.expiryDate;
    std::string convertedDate = convertDateFormat(ss.str());
    line += convertedDate + ",";
    line += std::to_string(smileResult.fwd) + ",";
    line += std::to_string(smileResult.atmvol) + ",";
    line += std::to_string(smileResult.bf25) + ",";
    line += std::to_string(smileResult.rr25) + ",";
    line += std::to_string(smileResult.bf10) + ",";
    line += std::to_string(smileResult.rr10) + "\n";

    std::cout << line; 
    //std::ofstream outputFile("output.csv", std::ios::app);

    if (outputFile.is_open()) {
        outputFile << line; // Write the line to the file
        //outputFile.close(); // Close the file
    }
    else {
        std::cout << "Error: Unable to open the file." << std::endl;
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: "
                  << argv[0] << " tick_data.csv" << " outputFile.csv" << std::endl;
        return 1;
    }
    const char* ticker_filename = argv[1];

    VolSurfBuilder<CubicSmile> volBuilder;
    std::ofstream outputFile("output.csv", std::ios::app);
    std::string line = "TIME, EXPIRY, FUT_PRICE, ATM, BF25, RR25, BF10, RR10\n";
    outputFile << line;

    auto feeder_listener = [&volBuilder] (const Msg& msg) {
        {
            volBuilder.Process(msg);
        }
    };

    auto timer_listener = [&volBuilder, &outputFile] (uint64_t now_ms) {
        // fit smile
        
        // TODO: stream the smiles and their fitting error to outputFile.csv 
        auto smiles = volBuilder.FitSmiles();

        // TODO: stream the smiles and their fitting error to outputFile.csv
        for (const auto& eachSmile: smiles){
            csvLineReport(eachSmile.second, outputFile);

        }


    };

    const auto interval = std::chrono::minutes(60);  // we call timer_listener at 1 minute interval
    CsvFeeder csv_feeder(ticker_filename,
                         feeder_listener,
                         interval,
                         timer_listener);


    while (csv_feeder.Step()) {
    }
    outputFile.close();
    return 0;
}