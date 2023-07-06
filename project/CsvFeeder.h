#ifndef QF633_CODE_CSVFEEDER_H
#define QF633_CODE_CSVFEEDER_H

#include <string>
#include <functional>
#include <chrono>
#include <fstream>

#include "Msg.h"

//Class for reading csv file without having to reread again on each
//msg update
class CsvFileBuffer {
private:
    std::ifstream& file;

    //this is used to hold to the next line read in csv
    std::vector<std::string> latestTokens;
    u_int64_t latestTime;
    bool eof_=false;

    //this loads data from row into this object's tokens_vector
    void loadTokens(const std::string& row, std::vector<std::string>& tokens_vector);
    void emplaceToMsg(Msg& msg, const std::vector<std::string>& tokens_vector);
public:
    bool eof(){
        return eof_;
    }
    bool readNextMsg(Msg& msg);
    CsvFileBuffer(std::ifstream& file);
};

class CsvFeeder {
public:
    using FeedListener = std::function<void(const Msg& msg)>;
    using TimerListener = std::function<void(uint64_t ms_now)>;
    CsvFeeder(const std::string ticker_filename,
              FeedListener feed_listener,
              std::chrono::minutes interval, TimerListener timer_listener);
    ~CsvFeeder();
    bool Step();

private:
    std::ifstream ticker_file_;
    FeedListener feed_listener_;
    const std::chrono::milliseconds interval_;
    TimerListener timer_listener_;
    CsvFileBuffer fileBuffer;

    uint64_t now_ms_{};
    Msg msg_;
    // your member variables and member functions below, if any
};

#endif //QF633_CODE_CSVFEEDER_H
