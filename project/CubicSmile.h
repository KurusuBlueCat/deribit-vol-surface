#ifndef _CUBICSMILE_H
#define _CUBICSMILE_H

#include "Date.h"
#include "Msg.h"

// CubicSpline interpolated smile, extrapolate flat
class CubicSmile
{
public:
    static FitSmileResult FitSmile(const datetime_t &expiryDate, const std::vector<TickData> &td); // FitSmile creates a Smile by fitting the smile params to the input tick data, it assume the tickData are of the same expiry
    // constructor, given the underlying price and marks, convert them to strike to vol pairs (strikeMarks), and construct cubic smile
    CubicSmile(double underlyingPrice, double T, double atmvol, double bf25, double rr25, double bf10, double rr10); // convert parameters to strikeMarks, then call BuildInterp() to create the cubic spline interpolator
    double Vol(double strike); // interpolate

private:
    void BuildInterp();
    // strike to implied vol marks
    std::vector<std::pair<double, double>> strikeMarks;
    std::vector<double> y2; // second derivatives
};

#endif
