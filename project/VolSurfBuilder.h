#ifndef QF633_CODE_VOLSURFBUILDER_H
#define QF633_CODE_VOLSURFBUILDER_H

#include "Msg.h"
#include "Date.h"
#include "Timer.h"
#include <sstream>
#include <string>
#include <map>

template<class Smile>
class VolSurfBuilder {
public:
    void Process(const Msg& msg);  // process message
    void PrintInfo();
    //std::map<datetime_t, std::pair<Smile, double> > FitSmiles();
    std::map<datetime_t, FitSmileResult > FitSmiles();
protected:
    // we want to keep the best level information for all instruments
    // here we use a map from contract name to BestLevelInfo, the key is contract name
    std::map<std::string, TickData> currentSurfaceRaw;
};


template<class Smile>
void VolSurfBuilder<Smile>::Process(const Msg& msg){
    std::map<std::string, TickData>::iterator iter;

    //if snap, just apply everything
    if (msg.isSnap){
        // std::cout << "Snapped" << std::endl;

        currentSurfaceRaw.clear();

        for (TickData tick : msg.Updates)
            currentSurfaceRaw[tick.ContractName] = tick;
        return;
    } 

    //is update, and individually checks each entry
    for (TickData tick : msg.Updates){
        // std::cout << "Updated" << std::endl;
        iter = currentSurfaceRaw.find(tick.ContractName);

        if (iter == currentSurfaceRaw.end()){
            //add entry if not found
            currentSurfaceRaw[tick.ContractName] = tick;
        } else if (currentSurfaceRaw[tick.ContractName].LastUpdateTimeStamp <= tick.LastUpdateTimeStamp){
            // add entry if current one is outdated
            currentSurfaceRaw[tick.ContractName] = tick;
        }
    }
}


/* template<class Smile>
void VolSurfBuilder<Smile>::PrintInfo(){
    std::cout << currentSurfaceRaw.size() << std::endl;
    for (auto tickIter=currentSurfaceRaw.begin(); tickIter!=currentSurfaceRaw.end(); ++tickIter){
        std:: cout << (tickIter->second).ContractName << std::endl;
    }
} */


template<class Smile>
void VolSurfBuilder<Smile>::PrintInfo(){
    std::cout << "==============^^^^^^^^^^^^^===============" << std::endl;
    std::cout << "==============<<PrintInfo>>===============" << std::endl;
    std::cout << "==============vvvvvvvvvvvvv===============" << std::endl;
    std::cout << "No. of contracts in memory: " << currentSurfaceRaw.size() << std::endl;
    std::map<datetime_t, std::vector<TickData> > tickersByExpiry{};
    for (auto tickIter=currentSurfaceRaw.begin(); tickIter!=currentSurfaceRaw.end(); ++tickIter){
        std::string ticker_name = (tickIter->second).ContractName;
        std::size_t hyphenPos = ticker_name.find('-');
        std::size_t secondHyphenPos = ticker_name.find('-', hyphenPos + 1);

        std::string expiry = ticker_name.substr(hyphenPos + 1, secondHyphenPos - hyphenPos - 1);
        datetime_t expiryDateTime = expiry;
        tickersByExpiry[expiryDateTime].push_back(tickIter->second);

    }

    std::cout << "No. of expiry observed: " << tickersByExpiry.size() << std::endl;

    for (auto tickIter2=tickersByExpiry.begin(); tickIter2!=tickersByExpiry.end(); ++tickIter2){
        
        uint64_t LatestUpdateTimeStamp = 0;
        double LatestUnderlyingPrice = 0;
        std::string UIndex;
        datetime_t dateNow; 

        for (auto tickIter3: tickIter2->second){
            if (LatestUpdateTimeStamp < tickIter3.LastUpdateTimeStamp){
            
                LatestUpdateTimeStamp = tickIter3.LastUpdateTimeStamp;
                LatestUnderlyingPrice = tickIter3.UnderlyingPrice;

            }
        }
        dateNow = LatestUpdateTimeStamp/1000;
        std::cout << std::endl;
        std::cout << (tickIter2->second)[0].UnderlyingIndex << std::endl;
        std::cout << "No. of Option Contracts: " << (tickIter2->second).size() << std::endl;
        std::cout << "Latest Epoch: " << LatestUpdateTimeStamp << std::endl;
        std::cout << "Underlying Price: " << LatestUnderlyingPrice << std::endl;

        std::cout << "Date Now: ";
        std::cout << dateNow.year << "-";
        std::cout << dateNow.month << "-";
        std::cout << dateNow.day << "-";
        std::cout << dateNow.hour << "-";
        std::cout << dateNow.min << "-";
        std::cout << dateNow.sec << std::endl;

        std::cout << "Expiry: ";
        std::cout << (tickIter2->first).year << "-";
        std::cout << (tickIter2->first).month << "-";
        std::cout << (tickIter2->first).day << "-";
        std::cout << (tickIter2->first).hour << "-";
        std::cout << (tickIter2->first).min << "-";
        std::cout << (tickIter2->first).sec << std::endl;

        auto dateDiff = (tickIter2->first) - dateNow;

        std::cout << "Time to expiry (Y): " << dateDiff << std::endl;
          
        }
} 


template <class Smile>
//std::map<datetime_t, std::pair<Smile, double> > VolSurfBuilder<Smile>::FitSmiles() {
std::map<datetime_t, FitSmileResult> VolSurfBuilder<Smile>::FitSmiles() {
    std::map<datetime_t, std::vector<TickData> > tickersByExpiry{};
    // TODO (Step 3): group the tickers in the current market snapshot by expiry date, and construct tickersByExpiry
    // ...
    for (auto tickIter=currentSurfaceRaw.begin(); tickIter!=currentSurfaceRaw.end(); ++tickIter){

        std::string ticker_name = (tickIter->second).ContractName;
        std::size_t hyphenPos = ticker_name.find('-');
        std::size_t secondHyphenPos = ticker_name.find('-', hyphenPos + 1);

        std::string expiry = ticker_name.substr(hyphenPos + 1, secondHyphenPos - hyphenPos - 1);
        datetime_t expiryDateTime = expiry;

        tickersByExpiry[expiryDateTime].push_back(tickIter->second);


    }

    std::map<datetime_t, FitSmileResult> res{};

    timer::TimingContext ctx;

    // then create Smile instance for each expiry by calling FitSmile() of the Smile
    for (auto iter = tickersByExpiry.begin(); iter != tickersByExpiry.end(); iter++) {
        if (iter->second.size() < 10) continue; //skip strike with low no of data

        FitSmileResult sm;
        std::string expiry;
        std::stringstream s;
        s << (iter->first);
        expiry = s.str();
        {
            timer::Timer timerFitSmile(ctx, expiry);

            sm = Smile::FitSmile(iter->first, iter->second);  // TODO: you need to implement FitSmile function in CubicSmile
        }


        sm.fitTimeMS = ctx.timings[expiry];

        double fittingError = 0;
        // TODO (Step 3): we need to measure the fitting error here
        std::cout << "Expiry: " << iter->first << std::endl;
        std::cout << "Contract Count: " << (iter->second).size() << std::endl;
        std::cout << "MSE: " << sm.smileError << std::endl;
        std::cout << "fwd: " << sm.fwd << "; ";
        std::cout << "T: " << sm.T << "; ";
        std::cout << "atmvol: " << sm.atmvol << "; ";
        std::cout << "bf25: " << sm.bf25 << "; ";
        std::cout << "rr25: " << sm.rr25 << "; ";
        std::cout << "bf10: " << sm.bf10 << "; ";
        std::cout << "rr10: " << sm.rr10 << "; " << std::endl;
        std::cout << "niter: " << sm.niter << "; " << std::endl;
        std::cout << "fitTimeMS: " << sm.fitTimeMS << "; " << std::endl;
        std::cout << "==================================" << std::endl;
        //res.insert(std::pair<datetime_t, std::pair<Smile, double> >(iter->first,std::pair<Smile, double>(sm, fittingError)));
        res.insert(std::pair<datetime_t, FitSmileResult >(iter->first, sm));
    }
    return res;
}

#endif //QF633_CODE_VOLSURFBUILDER_H
