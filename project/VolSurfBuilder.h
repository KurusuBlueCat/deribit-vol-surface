#ifndef QF633_CODE_VOLSURFBUILDER_H
#define QF633_CODE_VOLSURFBUILDER_H

#include <map>
#include "Msg.h"
#include "Date.h"

template<class Smile>
class VolSurfBuilder {
public:
    void Process(const Msg& msg);  // process message
    void PrintInfo();
    std::map<datetime_t, std::pair<Smile, double> > FitSmiles();
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
        std::cout << "Snapped" << std::endl;

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
    std::cout << currentSurfaceRaw.size() << std::endl;
    std::map<datetime_t, std::vector<TickData> > tickersByExpiry{};
    for (auto tickIter=currentSurfaceRaw.begin(); tickIter!=currentSurfaceRaw.end(); ++tickIter){
        std::string ticker_name = (tickIter->second).ContractName;
        std::size_t hyphenPos = ticker_name.find('-');
        std::size_t secondHyphenPos = ticker_name.find('-', hyphenPos + 1);

        std::string expiry = ticker_name.substr(hyphenPos + 1, secondHyphenPos - hyphenPos - 1);
        datetime_t expiryDateTime = expiry;
        tickersByExpiry[expiryDateTime].push_back(tickIter->second);

        
    }

    std::cout << tickersByExpiry.size() << std::endl;

    for (auto tickIter2=tickersByExpiry.begin(); tickIter2!=tickersByExpiry.end(); ++tickIter2){
            std::cout << (tickIter2->second).size() << std::endl;
        }
} 

template <class Smile>
std::map<datetime_t, std::pair<Smile, double> > VolSurfBuilder<Smile>::FitSmiles() {
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

    std::map<datetime_t, std::pair<Smile, double> > res{};
    // then create Smile instance for each expiry by calling FitSmile() of the Smile
    for (auto iter = tickersByExpiry.begin(); iter != tickersByExpiry.end(); iter++) {
        auto sm = Smile::FitSmile(iter->second);  // TODO: you need to implement FitSmile function in CubicSmile
        double fittingError = 0;
        // TODO (Step 3): we need to measure the fitting error here
        res.insert(std::pair<datetime_t, std::pair<Smile, double> >(iter->first,std::pair<Smile, double>(sm, fittingError)));
    }
    return res;
}

#endif //QF633_CODE_VOLSURFBUILDER_H
