#ifndef QF633_CODE_MSG_H
#define QF633_CODE_MSG_H

#include <cstdint>
#include <string>
#include <vector>

enum OptionType {Call, Put};
struct TickData {
    std::string ContractName;
    double BestBidPrice;
    double BestBidAmount;

    //this is quoted in 'percentage' IV of 50 is 50% or 0.5
    double BestBidIV;
    double BestAskPrice;
    double BestAskAmount;

    //this is quoted in 'percentage' IV of 50 is 50% or 0.5
    double BestAskIV;
    double MarkPrice;
    double MarkIV;
    std::string UnderlyingIndex;
    double UnderlyingPrice;
    double LastPrice;
    double OpenInterest;
    uint64_t LastUpdateTimeStamp;
    
    // TODO: Interpolate this from price
    // TODO: null and 0 might appear here, find a way to handle them
    //this is quoted in 'percentage' IV of 50 is 50% or 0.5
    double getMidIV() const {
        return (BestBidIV + BestAskIV)/2;
    }

    double GetStrike() const {
        std::size_t hyphenPos = ContractName.find('-');
        std::size_t secondHyphenPos = ContractName.find('-', hyphenPos + 1);
        std::size_t thirdHyphenPos = ContractName.find('-', secondHyphenPos + 1);

        auto strikeString = ContractName.substr(secondHyphenPos + 1, thirdHyphenPos - secondHyphenPos - 1);
        return std::stod(strikeString);
    }

    OptionType GetOptionType() const {
        return ContractName.back() == 'P' ?  OptionType::Put: OptionType::Call;
    }
};

//std::ostream& operator<< (std::ostream& os, const TickData& tick);

struct Msg {
    uint64_t timestamp{};
    bool isSnap;
    bool isSet = false;
    std::vector<TickData> Updates;
};

#endif //QF633_CODE_MSG_H
