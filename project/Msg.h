#ifndef QF633_CODE_MSG_H
#define QF633_CODE_MSG_H

#include "Date.h"
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

    TickData(const std::string& ContractName,
             double BestBidPrice,
             double BestBidAmount,
             double BestBidIV,
             double BestAskPrice,
             double BestAskAmount,
             double BestAskIV,
             double MarkPrice,
             double MarkIV,
             const std::string& UnderlyingIndex,
             double UnderlyingPrice,
             double LastPrice,
             double OpenInterest,
             uint64_t LastUpdateTimeStamp)
        : ContractName(ContractName),
          BestBidPrice(BestBidPrice),
          BestBidAmount(BestBidAmount),
          BestBidIV(BestBidIV),
          BestAskPrice(BestAskPrice),
          BestAskAmount(BestAskAmount),
          BestAskIV(BestAskIV),
          MarkPrice(MarkPrice),
          MarkIV(MarkIV),
          UnderlyingIndex(UnderlyingIndex),
          UnderlyingPrice(UnderlyingPrice),
          LastPrice(LastPrice),
          OpenInterest(OpenInterest),
          LastUpdateTimeStamp(LastUpdateTimeStamp)
    {}

    TickData() = default;
    
    // TODO: Interpolate this from price
    // TODO: null and 0 might appear here, find a way to handle them
    //this is quoted in 'percentage' IV of 50 is 50% or 0.5
    double getMidIV() const {
        return BestBidIV == 0 ? BestAskIV : 
               BestAskIV == 0 ? BestBidIV : 
                                (BestBidIV + BestAskIV)/2;
    }

    double getMidIVNaN() const {
        if ((BestBidIV == 0) || (BestAskIV) == 0)
            return 0;
        else
            return (BestBidIV + BestAskIV)/2;
    }

    double GetStrike() const {
        std::size_t hyphenPos = ContractName.find('-');
        std::size_t secondHyphenPos = ContractName.find('-', hyphenPos + 1);
        std::size_t thirdHyphenPos = ContractName.find('-', secondHyphenPos + 1);

        auto strikeString = ContractName.substr(secondHyphenPos + 1, thirdHyphenPos - secondHyphenPos - 1);
        return std::stod(strikeString);
    }

    std::string getExpiry() const {
        std::size_t hyphenPos = ContractName.find('-');
        std::size_t secondHyphenPos = ContractName.find('-', hyphenPos + 1);

        return ContractName.substr(hyphenPos + 1, secondHyphenPos - hyphenPos - 1);
    }

    OptionType GetOptionType() const {
        return ContractName.back() == 'P' ?  OptionType::Put: OptionType::Call;
    }

    bool isOTM(double underlyingPrice) const{
        bool strikeLowerThanPrice = this->GetStrike() < underlyingPrice;
        bool isPut = this->GetOptionType() == OptionType::Put;
        return strikeLowerThanPrice == isPut;
    }

    bool isOTM() const {
        return this->isOTM(this->UnderlyingPrice);
    }
};

//std::ostream& operator<< (std::ostream& os, const TickData& tick);

struct Msg {
    uint64_t timestamp{};
    bool isSnap;
    // bool isSet = false;
    std::vector<TickData> Updates;
};

// //Added by Lora on Sat
// std::string Expirydate = getExpiry();

// struct Date_Time{    
//     std::string Expirydate; 
// }

struct FitSmileResult {
    double smileError;
    double fwd;
    double T;
    double atmvol;
    double bf25;
    double rr25;
    double bf10;
    double rr10;
    int niter;
    uint64_t LastUpdateTimeStamp;
    datetime_t expiryDate;
    int contractCount;
    double fitTimeMS;

    static FitSmileResult getInvalid(){
        return {0,0,0,0,0,0,0,0};
    }
};

std::ostream& operator<< (std::ostream& os, const TickData& tick);

#endif //QF633_CODE_MSG_H
