#pragma once

#include <Eigen/Core>
#include <LBFGS.h>

//module for creating functions that can calculate both values and gradients
//on each parameter in a modular fashion.
//Each of the class included in this namespace can be instantiated with
//a function and a stepsize, and it will returns a new functor.
namespace ngrad{

//Central difference with O(h^2) truncation error on gradient.
//Requires extra 2*N evaluation, N=number of parameters
template<class T>
class FirstCentralDiff
{
private:
    double diff_;
public:
    T& func;

    FirstCentralDiff(T& func, double diff) 
        : func(func),
          diff_(diff)
    {}

    double operator()(const Eigen::VectorXd& x, Eigen::VectorXd& grad)
    {
        int dim = x.size();
        double fLeft1, fRight1;
        Eigen::VectorXd inputVector = x;
        for(int i=0; i<dim; ++i){
            inputVector[i] += diff_;
            fRight1 = func(inputVector);
            inputVector[i] -= 2*diff_;
            fLeft1 = func(inputVector);
            inputVector[i] += diff_;
            grad[i] = (fRight1 - fLeft1)/(2*diff_);
        }
        return func(x);
    }
};

//Forward difference with O(h) truncation error on gradient.
//Requires extra N evaluation, N=number of parameters
template<class T>
class FirstForwardDifference
{
private:
    double diff_;
public:
    T& func;

    FirstForwardDifference(T& func, double diff) 
        : func(func),
          diff_(diff)
    {}

    double operator()(const Eigen::VectorXd& x, Eigen::VectorXd& grad)
    {
        int dim = x.size();
        double fcenter = func(x);
        double fRight;
        Eigen::VectorXd inputVector = x;
        for(int i=0; i<dim; ++i){
            inputVector[i] += diff_;
            fRight = func(inputVector);
            inputVector[i] -= diff_;
            grad[i] = (fRight - fcenter)/(diff_);
        }
        return fcenter;
    }
};

}