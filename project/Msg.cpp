#include <iostream>
#include "Msg.h"

std::ostream& operator<< (std::ostream& os, const TickData& tick){
    std::string contractName = tick.ContractName; // Convert to string
    os << tick.ContractName << ", " << tick.BestBidAmount << ", " << tick.BestAskAmount;
    return os;
}