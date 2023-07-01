#pragma once

#include <Eigen/Core>
#include <LBFGS.h>

namespace ngrad{

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

}