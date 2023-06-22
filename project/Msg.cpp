#include <iostream>
#include "Msg.h"

std::ostream& operator<< (std::ostream& os, const TickData& tick){
    os << tick.ContractName << ", " << tick.BestBidAmount << ", " << tick.BestAskAmount;
    return os;
}