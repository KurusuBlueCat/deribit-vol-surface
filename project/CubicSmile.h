#ifndef _CUBICSMILE_H
#define _CUBICSMILE_H

#include "BSAnalytics.h"
#include "Date.h"
#include "Msg.h"
#include <iostream>
#include <cmath>
#include <map>
#include <cmath>
#include <tuple>
#include <array>
#include <Eigen/Core>
#include <LBFGS.h>
#include "GradientMaker.h"

using namespace std;


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
    vector<pair<double, double>> strikeMarks;
    vector<double> y2; // second derivatives
};

// not within the class
tuple<uint64_t, double> getLatestTimeAndPrice(const std::vector<TickData> &volTickerSnap)
{
    double fwd;
    uint64_t LatestUpdateTimeStamp = 0;
    for (auto &iter : volTickerSnap)
    {
        if (LatestUpdateTimeStamp < iter.LastUpdateTimeStamp)
        {
            LatestUpdateTimeStamp = iter.LastUpdateTimeStamp;
            fwd = iter.UnderlyingPrice;
        }
    }
    return {LatestUpdateTimeStamp, fwd};
}



CubicSmile::CubicSmile(double underlyingPrice, double T, double atmvol, double bf25, double rr25, double bf10, double rr10)
{
    // convert delta marks to strike vol marks, setup strikeMarks, then call BUildInterp
    double v_qd90 = atmvol + bf10 - rr10 / 2.0;
    double v_qd75 = atmvol + bf25 - rr25 / 2.0;
    double v_qd25 = atmvol + bf25 + rr25 / 2.0;
    double v_qd10 = atmvol + bf10 + rr10 / 2.0;

    // we use quick delta: qd = N(log(F/K / (atmvol) / sqrt(T))
    double stdev = atmvol * sqrt(T);
    double k_qd90 = quickDeltaToStrike(0.9, underlyingPrice, stdev);
    double k_qd75 = quickDeltaToStrike(0.75, underlyingPrice, stdev);
    double k_qd25 = quickDeltaToStrike(0.25, underlyingPrice, stdev);
    double k_qd10 = quickDeltaToStrike(0.10, underlyingPrice, stdev);

    //this method will result in 5 strike vol marks anyway
    strikeMarks.reserve(5); 
    // OPT: replaced push_back with emplace_back
    strikeMarks.emplace_back(std::pair<double, double>(k_qd90, v_qd90));
    strikeMarks.emplace_back(std::pair<double, double>(k_qd75, v_qd75));
    strikeMarks.emplace_back(std::pair<double, double>(underlyingPrice, atmvol));
    strikeMarks.emplace_back(std::pair<double, double>(k_qd25, v_qd25));
    strikeMarks.emplace_back(std::pair<double, double>(k_qd10, v_qd10));
    BuildInterp();
}

void CubicSmile::BuildInterp()
{
    int n = strikeMarks.size();
    // end y' are zero, flat extrapolation
    double yp1 = 0;
    double ypn = 0;
    y2.resize(n);
    vector<double> u(n - 1);

    y2[0] = -0.5;
    u[0] = (3.0 / (strikeMarks[1].first - strikeMarks[0].first)) *
            ((strikeMarks[1].second - strikeMarks[0].second) / (strikeMarks[1].first - strikeMarks[0].first) - yp1);

    for (int i = 1; i < n - 1; i++)
    {
        double sig = (strikeMarks[i].first - strikeMarks[i - 1].first) / (strikeMarks[i + 1].first - strikeMarks[i - 1].first);
        double p = sig * y2[i - 1] + 2.0;
        y2[i] = (sig - 1.0) / p;
        u[i] = (strikeMarks[i + 1].second - strikeMarks[i].second) / (strikeMarks[i + 1].first - strikeMarks[i].first) - (strikeMarks[i].second - strikeMarks[i - 1].second) / (strikeMarks[i].first - strikeMarks[i - 1].first);
        u[i] = (6.0 * u[i] / (strikeMarks[i + 1].first - strikeMarks[i - 1].first) - sig * u[i - 1]) / p;
    }

    double qn = 0.5;
    double un = (3.0 / (strikeMarks[n - 1].first - strikeMarks[n - 2].first)) *
                (ypn - (strikeMarks[n - 1].second - strikeMarks[n - 2].second) / (strikeMarks[n - 1].first - strikeMarks[n - 2].first));

    y2[n - 1] = (un - qn * u[n - 2]) / (qn * y2[n - 2] + 1.0);

    for (int i = n - 2; i >= 0; i--)
    {
        y2[i] = y2[i] * y2[i + 1] + u[i];
        //    std::cout << "y2[" << i << "] = " << y2[i] << std::endl;
    }
}

double CubicSmile::Vol(double strike)
    {
    unsigned i;
    // we use trivial search, but can consider binary search for better performance
    for (i = 0; i < strikeMarks.size(); i++)
        if (strike < strikeMarks[i].first)
            break; // i stores the index of the right end of the bracket

    // extrapolation
    if (i == 0)
        return strikeMarks[i].second;
    if (i == strikeMarks.size())
        return strikeMarks[i - 1].second;

    // interpolate
    double h = strikeMarks[i].first - strikeMarks[i - 1].first;
    double a = (strikeMarks[i].first - strike) / h;
    double b = 1 - a;
    double c = (a * a * a - a) * h * h / 6.0;
    double d = (b * b * b - b) * h * h / 6.0;
    return a * strikeMarks[i - 1].second + b * strikeMarks[i].second + c * y2[i - 1] + d * y2[i];
    }

class smile_MSE
{
    private:
        std::map<double, double> MIV;
        int n;
        double fwd, T;
        double computeMSE(const std::map<double, double>& MIV, const Eigen::VectorXd& x){
            double fx = 0.0, N = 0;
            CubicSmile csCandidate(fwd, T, x[0], x[1], x[2], x[3], x[4]);

            for (const auto& kVolPair: MIV){
                N++;
                double err = csCandidate.Vol(kVolPair.first) - kVolPair.second;
                fx += err*err;
            }

            return fx/N;
        }


    public:

        smile_MSE(int _n, std::map<double, double> _MIV, double _fwd, double _T) : 
        n(_n),
        MIV(_MIV), 
        fwd(_fwd), 
        T(_T) {}

        double operator()(const Eigen::VectorXd& x, Eigen::VectorXd& grad)
        {
            double fx = computeMSE(MIV, x);
            
            double fLeft1, fRight1;
            Eigen::VectorXd inputVector = x;

            for(int i=0; i<n; ++i){
                inputVector[i] += 0.001;
                fRight1 = computeMSE(MIV, inputVector);
                inputVector[i] -= 2*0.001;
                fLeft1 = computeMSE(MIV, inputVector);
                inputVector[i] += 0.001;
                grad[i] = (fRight1 - fLeft1)/(2*0.001);
            }

            return fx;
        }
        
};

FitSmileResult CubicSmile::FitSmile(const datetime_t &expiryDate, const std::vector<TickData> &volTickerSnap)
{
    // volTickerSnap must only include OTM options

    // - get latest underlying price from all tickers based on LastUpdateTimeStamp
    //structured binding. datetime_t T, double fwd;
    auto [latestTime, fwd] = getLatestTimeAndPrice(volTickerSnap);

    // - get time to expiry T
    double T = (expiryDate - (latestTime / 1000)) < 0 ? 0 : (expiryDate - (latestTime / 1000));

    // std::cout << latestTime << std::endl;
    // std::cout << T << std::endl;

    // TODO (step 3): fit a CubicSmile that is close to the raw tickers
    // - make sure all tickData are on the same expiry and same underlying

    //map of strike to OTM 
    //This is guaranteed ascending ordered by strike
    //This is great, because we can use the ordered property later
    std::map<double, double> strikeImpliedVol;

    for (auto &td : volTickerSnap)
    {
        
        if (td.isOTM(fwd))  //Only keep OTM option
            strikeImpliedVol[td.GetStrike()] = td.getMidIV();

        // std::cout << td.getMidIV() << ": " << td.GetStrike() << std::endl;
    }

    // 1. TODO:
    // We estimate 5 param using 5 closest iv to a given delta

    std::vector<double> threshVector = {0.9, 0.75, 0.5, 0.25, 0.1};

    std::vector<double> ivVector;
    ivVector.reserve(threshVector.size());
    {
        auto kVolPair = strikeImpliedVol.begin();
        double qd = quickDelta(fwd, kVolPair->first, kVolPair->second, T);

        for (auto threshPtr=threshVector.begin(); threshPtr!=threshVector.end(); ++threshPtr){
            //we can increment through strikeImpliedVol as strike is guaranteed increasing
            //therefore, qd is guaranteed decreasing
            while ((kVolPair!=strikeImpliedVol.end()) //while there's still kVolPair in the list
                   && (qd > (*threshPtr)))  //and quick delta is still greater than our threshold
            {
                //use IV of the contract itself. ATMVol does not make too much of a difference
                //And we are trying to get first estimate, not an exact answer.
                qd = quickDelta(fwd, kVolPair->first, kVolPair->second, T);
                ++kVolPair;
            }
            // std::cout << qd << std::endl;
            //add IV of first contract that is less than threshold
            ivVector.emplace_back(kVolPair->second);
        }
    }

    const double& v_qd90 = ivVector[0];
    const double& v_qd75 = ivVector[1];
    const double& atmvol = ivVector[2];
    const double& v_qd25 = ivVector[3];
    const double& v_qd10 = ivVector[4];


    double bf10 = (v_qd10 + v_qd90)/2 - atmvol;
    double rr10 = v_qd10 - v_qd90;
    double bf25 = (v_qd25 + v_qd75)/2 - atmvol;
    double rr25 = v_qd25 - v_qd75;

    // 2. TODO:
    const int n = 5;  
    // Set up parameters
    LBFGSpp::LBFGSParam<double> param;
    param.epsilon = 1e-6;
    param.max_iterations = 100;

     // Create solver and function object
    LBFGSpp::LBFGSSolver<double> solver(param);
    smile_MSE fun(n, strikeImpliedVol, fwd, T);

    Eigen::VectorXd x {{atmvol, bf25, rr25, bf10, rr10}}; 
    double fx;
    int niter = solver.minimize(fun, x, fx);

    // optimized values
    double atmvolOpt = x[0], bf25Opt = x[1], rr25Opt = x[2], bf10Opt = x[3], rr10Opt = x[4];
    

    FitSmileResult res;
    res.smileError = fx;
    res.fwd = fwd;
    res.T = T;
    res.atmvol = atmvolOpt;
    res.bf25 = bf25Opt;
    res.rr25 = rr25Opt;
    res.bf10 = bf10Opt;
    res.rr10 = rr10Opt;

    return res;


    //return {fwd, T, atmvol, bf25, rr25, bf10, rr10};
}

#endif
