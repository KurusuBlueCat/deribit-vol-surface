#include "BSAnalytics.h"
#include "Date.h"
#include "Msg.h"
#include "GradientMaker.h"
#include "CubicSmile.h"
#include "VolSurfBuilder.h"
#include <iostream>
#include <cmath>
#include <map>
#include <cmath>
#include <tuple>
#include <string>
#include <array>
#include <Eigen/Core>
#include <LBFGS.h>
#include <LBFGSB.h>
#include <fstream>

template class VolSurfBuilder<CubicSmile>;

// not within the class
std::tuple<uint64_t, double> getLatestTimeAndPrice(const std::vector<TickData> &volTickerSnap)
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

    // std::cout << k_qd90 << " " << k_qd75 << " " << underlyingPrice << " " << k_qd25 << " " << k_qd10 << std::endl;

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
    std::vector<double> u(n - 1);

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

namespace constants{
const double inf = std::numeric_limits<double>::infinity();
//some bound to prevent weird butterfly values
const Eigen::VectorXd lb {{-inf, 0, -inf, 0, -inf}};
const Eigen::VectorXd ub {{inf, inf, inf, inf, inf}};
}

class smile_MSE
{
    private:
        datetime_t Expiry;
        std::map<double, std::pair<double, double>> MIV;
        double fwd, T;
        bool print;

    public:

        smile_MSE(datetime_t expiryDate, std::map<double, std::pair<double, double>> _MIV, double _fwd, double _T, bool print=false) 
            : Expiry(expiryDate),
              MIV(_MIV), 
              fwd(_fwd), 
              T(_T),
              print(print)
        {}

        double operator()(const Eigen::VectorXd& x)
        {
            double fx = 0.0, N = 0;
            CubicSmile csCandidate(fwd, T, x[0], x[1], x[2], x[3], x[4]);
            size_t totalIterations = MIV.size();
            size_t currentIteration = 0;
            



            for (const auto& kVolPair: MIV){
                if (print){
                    std::ofstream outputFile("output_analytics.csv", std::ios::app);   
                    if (currentIteration == 0){
                        std::string line = "Expiry, T, Strike, Result, Target, Weight\n";
                        outputFile << line;
                    }

                    std::string line = std::to_string(Expiry.year)+"-"+std::to_string(Expiry.month)+"-"+std::to_string(Expiry.day) + ",";
                    line += std::to_string(T) + ",";
                    line += std::to_string(kVolPair.first) + ",";
                    line += std::to_string(csCandidate.Vol(kVolPair.first)) + ",";
                    line += std::to_string(kVolPair.second.first ) + ",";
                    line += std::to_string(kVolPair.second.second) + "\n";

                    outputFile << line;

                    ++currentIteration;
                    bool isLastIteration = (currentIteration == totalIterations);
                    outputFile.close();

                }

                if (kVolPair.second.second == 0) //skip unweighted samples
                    continue;

                ++N;
                double err = csCandidate.Vol(kVolPair.first) - kVolPair.second.first;
                err *= kVolPair.second.second; //weighted error
                fx += err*err;
            }

            return fx/N;
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

    if (T == 0)
        return FitSmileResult::getInvalid(expiryDate);

    // std::cout << latestTime << std::endl;
    // std::cout << T << std::endl;

    // TODO (step 3): fit a CubicSmile that is close to the raw tickers
    // - make sure all tickData are on the same expiry and same underlying

    std::map<double, std::pair<double, double>> strikeIVWeight;

    double atmvol;

    for (const auto &td : volTickerSnap)
    {
        
        if (td.isOTM(fwd))  //Only keep OTM option
        {
            double strike = td.GetStrike();
            
            if (strike < fwd) //use the first OTM lower than strike as "ATM"
                atmvol = td.getMidIVNaN();

            strikeIVWeight[td.GetStrike()] = {td.getMidIVNaN(), (td.getMidIVNaN() >0) ? 1 : 0};
        }
        // std::cout << td.getMidIV() << ": " << td.GetStrike() << std::endl;
    }

    int contractCount = 0;
    for (auto& kIVWeight: strikeIVWeight){
        if (quickDelta(fwd, kIVWeight.first, atmvol, T) > 0.95)
            kIVWeight.second.second = 0; //ignore contracts with qd > 0.95
        else if (quickDelta(fwd, kIVWeight.first, atmvol, T) < 0.05)
            kIVWeight.second.second = 0; //ignore contracts with qd < 0.05
        else {
            ++contractCount;
        }
    }
 
    std::cout << contractCount << std::endl;

    // 1. TODO:
    // We estimate 5 param using 5 closest iv to a given delta
    //this does not seem to be worthwhile



    // 2. TODO: 
    // Set up parameters
    LBFGSpp::LBFGSBParam<double> param;
    param.epsilon = 1e-6;
    param.max_iterations = 100;

     // Create solver and function object
    LBFGSpp::LBFGSBSolver<double> solver(param);
    smile_MSE fun(expiryDate, strikeIVWeight, fwd, T);
    smile_MSE funWPrint(expiryDate, strikeIVWeight, fwd, T, true);
    // ngrad::FirstCentralDiff funWithGrad(fun, 0.00001);
    ngrad::FirstForwardDifference funWithGrad(fun, 0.00001);

    // std::cout << bf25 << " " << rr25 << " " << bf10 << " " << rr10 << std::endl;

    Eigen::VectorXd x {{atmvol, 0.01, -0.14, 0.1, -0.2}}; //heuristic estimates
    double fx;

    int niter;
    try{
    niter = solver.minimize(funWithGrad, x, fx, constants::lb, constants::ub);
    } catch (std::runtime_error) {
        return FitSmileResult::getInvalid(expiryDate);
    }

    // for (const auto& v : strikeIVWeight){
    //     std::cout << v.first << " " << std::get<0>(v.second) << std::endl;
    // }

    funWPrint(x);

    // optimized values
    double atmvolOpt = x[0], bf25Opt = x[1], rr25Opt = x[2], bf10Opt = x[3], rr10Opt = x[4];


    //we can pass the above directly back to be instantiated by the previous stack

    return {fx, fwd, T, x[0], x[1], x[2], x[3], x[4], niter, latestTime, expiryDate, contractCount};

    //return {fwd, T, atmvol, bf25, rr25, bf10, rr10};
}