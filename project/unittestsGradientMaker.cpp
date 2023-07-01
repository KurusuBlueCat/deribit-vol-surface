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
void testNd(int n, double tol){
    Eigen::VectorXd x = Eigen::VectorXd::Random(n);

    Eigen::VectorXd grad(n);
    Rosenbrock fun(n);
    fun(x, grad);

    Diff numFun(fun, 0.0001);
    Eigen::VectorXd gradNum(n);
    numFun(x, gradNum);

    double pctAbsError = ((gradNum - grad).array()/grad.array()).cwiseAbs().mean();
    std::cout << "Pct Error: " << pctAbsError*100 << "%" << std::endl;
    assert(pctAbsError < tol);
    std::cout << n << "D rosenbrock passed. 1 Gradient and Error calculation took " << fun.errOnlyCount << " evaluation." << std::endl;
}

int main(){
    std::cout << "FirstCentralDiff test" << std::endl;
    for(int i =2; i < 17; i+=2){
        testNd<ngrad::FirstCentralDiff>(i, 0.000001);
    }

    std::cout << "FirstForwardDifference test" << std::endl;
    for(int i =2; i < 17; i+=2){
        testNd<ngrad::FirstForwardDifference>(i, 0.01);
    }
}