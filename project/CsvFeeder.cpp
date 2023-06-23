#include <iostream>
#include "CsvFeeder.h"
#include "date/date.h"
#include "Date.h"

uint64_t TimeToUnixMS(std::string ts) {
    std::istringstream in{ts};
    std::chrono::system_clock::time_point tp;
    in >> date::parse("%FT%T", tp);
    const auto timestamp = std::chrono::time_point_cast<std::chrono::milliseconds>(tp).time_since_epoch().count();
    return timestamp;
}

double ParseValue(const std::string& value) {
    return value.empty() ? std::numeric_limits<double>::quiet_NaN() : std::stod(value);
}

/* datetime_t GetExpiry(const std::string& contractName){
    std::string ticker_name = contractName;
    std::size_t hyphenPos = ticker_name.find('-');
    std::size_t secondHyphenPos = ticker_name.find('-', hyphenPos + 1);

    std::string expiry = ticker_name.substr(hyphenPos + 1, secondHyphenPos - hyphenPos - 1);
    datetime_t expiryDateTime = expiry;

    return expiryDateTime;
} */

bool ReadNextMsg(std::ifstream& file, Msg& msg) {
    if (file.eof()) {
        return false;
    }
    // START of TODO:

    while (true) {

        // Read the current position of the file
        std::streampos current_pos = file.tellg();

        // Read rows and skip header row
        std::string row;
        std::getline(file, row);
        std::string headers = "contractName,time,msgType,priceCcy,bestBid,bestBidAmount,bestBidIV,bestAsk,bestAskAmount,bestAskIV,markPrice,markIV,underlyingIndex,underlyingPrice,interest_rate,lastPrice,open_interest,vega,theta,rho,gamma,delta";
        if (row.find(headers) != std::string::npos) {
            std::getline(file, row);
        }

        std::istringstream iss(row);
        std::vector<std::string> tokens_vector;
        std::string token;

        while (std::getline(iss, token, ',')) {
            tokens_vector.emplace_back(std::move(token));
        }

        // Check for new messsages
        uint64_t check_timestamp = TimeToUnixMS(tokens_vector[1]);
        if (msg.timestamp == 0) {
            msg.timestamp = check_timestamp;
        } 

        // if msg.timestamp == check_timestamp then this check is skipped
        else if (msg.timestamp != check_timestamp) {
            // Reset msg and clear all message data if msg has been set.
            if (msg.isSet) {
                msg.timestamp = check_timestamp;
                msg.isSet = false;
                msg.isSnap = false;
                msg.Updates.clear();
            } else {
                msg.isSet = true;
                // Else, move pointer back to the previous location
                file.seekg(current_pos);
                return true;
            }
        }

        std::string msgType = tokens_vector[2];
        if (msgType.find("snap") != std::string::npos && !msg.isSnap) {
            msg.isSnap = true;
            msg.Updates.clear();
        }

        TickData tick_data;
        tick_data.ContractName = tokens_vector[0];
        tick_data.BestBidPrice = ParseValue(tokens_vector[4]);
        tick_data.BestBidAmount = ParseValue(tokens_vector[5]);
        tick_data.BestBidIV = ParseValue(tokens_vector[6]);
        tick_data.BestAskPrice = ParseValue(tokens_vector[7]);
        tick_data.BestAskAmount = ParseValue(tokens_vector[8]);
        tick_data.BestAskIV = ParseValue(tokens_vector[9]);
        tick_data.MarkPrice = ParseValue(tokens_vector[10]);
        tick_data.MarkIV = ParseValue(tokens_vector[11]);
        tick_data.UnderlyingIndex = tokens_vector[12];
        tick_data.UnderlyingPrice = ParseValue(tokens_vector[13]);
        tick_data.LastPrice = ParseValue(tokens_vector[15]);
        tick_data.OpenInterest = ParseValue(tokens_vector[16]);
        tick_data.LastUpdateTimeStamp = tokens_vector[1].empty() ? std::numeric_limits<double>::quiet_NaN() : TimeToUnixMS(tokens_vector[1]);
        msg.Updates.push_back(tick_data);

        // std::cout << tick_data << std::endl;
    }
    //END OF TODO
    return true;
}


CsvFeeder::CsvFeeder(const std::string ticker_filename,
                     FeedListener feed_listener,
                     std::chrono::minutes interval,
                     TimerListener timer_listener)
        : ticker_file_(ticker_filename),
          feed_listener_(feed_listener),
          interval_(interval),
          timer_listener_(timer_listener) {
    // initialize member variables with input information, prepare for Step() processing

    ReadNextMsg(ticker_file_, msg_);
    if (msg_.isSet) {
        // initialize interval timer now_ms_
        now_ms_ = msg_.timestamp;
    } else {
        throw std::invalid_argument("empty message at initialization");
    }
}

bool CsvFeeder::Step() {
    if (msg_.isSet) {
        // call feed_listener with the loaded Msg
        feed_listener_(msg_);

        // if current message's timestamp is crossing the given interval, call time_listener, change now_ms_ to the next interval cutoff
        if (now_ms_ < msg_.timestamp) {
            timer_listener_(now_ms_);
            now_ms_ += interval_.count();
        }
        // load tick data into Msg
        // if there is no more message from the csv file, return false, otherwise true
        return ReadNextMsg(ticker_file_, msg_);
    }
    return false;
}

CsvFeeder::~CsvFeeder() {
    // release resource allocated in constructor, if any
}