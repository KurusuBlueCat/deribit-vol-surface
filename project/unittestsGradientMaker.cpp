#include "GradientMaker.h"
#include <iostream>
#include <cassert>

class Rosenbrock
{
private:
    int n;
public:
    int analyticCount=0;
    int errOnlyCount=0;
    Rosenbrock(int n_) : n(n_) {}
    double operator()(const Eigen::VectorXd& x, Eigen::VectorXd& grad)
    {
        // std::cout << "Gradient version called" << std::endl;
        ++analyticCount;
        double fx = 0.0;
        for(int i = 0; i < n; i += 2)
        {
            double t1 = 1.0 - x[i];
            double t2 = 10 * (x[i + 1] - x[i] * x[i]);
            grad[i + 1] = 20 * t2;
            grad[i]     = -2.0 * (x[i] * grad[i + 1] + t1);
            fx += t1 * t1 + t2 * t2;
        }
        assert( ! std::isnan(fx) );
        return fx;
    }

    double operator()(const Eigen::VectorXd& x)
    {
        // std::cout << "Non-gradient version called" << std::endl;
        ++errOnlyCount;
        double fx = 0.0;
        for(int i = 0; i < n; i += 2)
        {
            double t1 = 1.0 - x[i];
            double t2 = 10 * (x[i + 1] - x[i] * x[i]);
            fx += t1 * t1 + t2 * t2;
        }
        assert( ! std::isnan(fx) );
        return fx;
    }
};

template<template<class Func> class Diff>
void testNd(int n){
    Eigen::VectorXd x = Eigen::VectorXd::Random(n);

    Eigen::VectorXd grad(n);
    Rosenbrock fun(n);
    fun(x, grad);

    Diff numFun(fun, 0.001);
    Eigen::VectorXd gradNum(n);
    numFun(x, gradNum);

    double pctAbsError = ((gradNum - grad).array()/grad.array()).cwiseAbs().mean();
    std::cout << "Pct Error: " << pctAbsError*100 << "%" << std::endl;
    assert(pctAbsError < 0.0001);
    std::cout << n << "D rosenbrock passed. 1 Gradient and Error calculation took " << fun.errOnlyCount << " evaluation." << std::endl;
}

int main(){
    for(int i =2; i < 17; i+=2){
        testNd<ngrad::FirstCentralDiff>(i);
    }
}