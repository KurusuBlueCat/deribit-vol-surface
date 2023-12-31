#include <iostream>
#include "CsvFeeder.h"
#include "date/date.h"
#include "Date.h"

namespace constants{
    const std::string headers = "contractName,time,msgType,priceCcy,bestBid,bestBidAmount,bestBidIV,bestAsk,bestAskAmount,bestAskIV,markPrice,markIV,underlyingIndex,underlyingPrice,interest_rate,lastPrice,open_interest,vega,theta,rho,gamma,delta";
}

//converts timestamp to epoch
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

void CsvFileBuffer::loadTokens(const std::string& row, std::vector<std::string>& tokens_vector)
{
    tokens_vector.clear();
    std::istringstream iss(row);
    std::string token;

    while (std::getline(iss, token, ',')) {
        tokens_vector.emplace_back(std::move(token));
    }

    latestTime = tokens_vector[1].empty() ? std::numeric_limits<double>::quiet_NaN() : TimeToUnixMS(tokens_vector[1]);
    // std::cout << tokens_vector[0] << std::endl;
    // std::cout << latestTime << std::endl;
}

//instantiate latest TickData in-place to msg.Updates with tokens_vector
//via emplace_back method
void CsvFileBuffer::emplaceToMsg(Msg& msg, const std::vector<std::string>& tokens_vector)
{
    msg.Updates.emplace_back(
        tokens_vector[0], //tick_data.ContractName
        ParseValue(tokens_vector[4]), //tick_data.BestBidPrice
        ParseValue(tokens_vector[5]), //tick_data.BestBidAmount
        ParseValue(tokens_vector[6])/100, //tick_data.BestBidIV divide vol by 100 right away
        ParseValue(tokens_vector[7]), //tick_data.BestAskPrice
        ParseValue(tokens_vector[8]), //tick_data.BestAskAmount
        ParseValue(tokens_vector[9])/100, // tick_data.BestAskIV divide vol by 100 right away
        ParseValue(tokens_vector[10]), //tick_data.MarkPrice
        ParseValue(tokens_vector[11]), //tick_data.MarkIV
        tokens_vector[12], //tick_data.UnderlyingIndex
        ParseValue(tokens_vector[13]), //tick_data.UnderlyingPrice
        ParseValue(tokens_vector[15]), //tick_data.LastPrice
        ParseValue(tokens_vector[16]), //tick_data.OpenInterest
        latestTime //tick_data.LastUpdateTimeStamp
    );
}

CsvFileBuffer::CsvFileBuffer(std::ifstream& file)
    : file(file),
      eof_(false)
{
    std::string row;
    std::getline(file, row); //skip header by reading once
    std::getline(file, row); 
    loadTokens(row, latestTokens);
}

bool CsvFileBuffer::readNextMsg(Msg& msg)
{
    msg.Updates.clear();

    if (eof_)
        // std::cout << "Here";
        return false;

    //push previous buffered token into msg
    emplaceToMsg(msg, latestTokens);
    //also update to latest time
    msg.timestamp = msg.Updates.back().LastUpdateTimeStamp;
    msg.isSnap = (latestTokens[2].find("snap") != std::string::npos);

    // START of TODO:

    while (true) {
        if (file.eof())
        {
            //mark eof
            eof_ = true;
            return false;
        }
        
        std::string row;
        // Read rows
        std::getline(file, row);
        //load to latest tokens
        loadTokens(row, latestTokens);

        // do not push yet if timestamp is not the same
        //this will leave latestTokens to be emplaced in the next loop
        if (msg.timestamp != latestTime) {
            return true;
        }

        const std::string& msgType = latestTokens[2];

        //skips this check if the current msg is already a snap
        if (!msg.isSnap && msgType.find("snap") != std::string::npos) {
            msg.isSnap = true;
            //clear any previous tick data in the same time after a snap is detected
            msg.Updates.clear();
        }

        emplaceToMsg(msg, latestTokens);
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
          timer_listener_(timer_listener),
          fileBuffer(ticker_file_)
    {
    // initialize member variables with input information, prepare for Step() processing
    bool success = fileBuffer.readNextMsg(msg_);

    // std::cout << msg_.Updates.front() << std::endl;

    if (success) {
        // initialize interval timer now_ms_
        now_ms_ = msg_.timestamp;
    } else {
        throw std::invalid_argument("Failed to read at initialization");
    }
}

bool CsvFeeder::Step() {

    // call feed_listener with the loaded Msg
    feed_listener_(msg_);

    // if current message's timestamp is crossing the given interval, call time_listener, change now_ms_ to the next interval cutoff
    if (now_ms_ < msg_.timestamp) {
        timer_listener_(now_ms_);
        now_ms_ += interval_.count();
    }
    // load tick data into Msg
    // if there is no more message from the csv file, return false, otherwise true
    return fileBuffer.readNextMsg(msg_);

    // return false;
}

CsvFeeder::~CsvFeeder() {
    // release resource allocated in constructor, if any
}