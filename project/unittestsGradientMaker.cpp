#include "GradientMaker.h"
#include <iostream>
#include <cassert>

class Rosenbrock
{
private:
    int n;
public:
    Rosenbrock(int n_) : n(n_) {}
    double operator()(const Eigen::VectorXd& x, Eigen::VectorXd& grad)
    {
        std::cout << "Gradient version called" << std::endl;
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
        std::cout << "Non-gradient version called" << std::endl;
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

int main(){
    Eigen::VectorXd x(2);
    x[0] = 0;
    x[1] = 1;
    Eigen::VectorXd grad(2);
    Rosenbrock fun(2);
    fun(x, grad);

    for (auto v: grad){
        std::cout << v << " ";
    }
    std::cout << std::endl;

    ngrad::FirstCentralDiff numFun(fun, 0.001);

    Eigen::VectorXd xNum(2);
    xNum[0] = 0;
    xNum[1] = 1;
    Eigen::VectorXd gradNum(2);

    numFun(xNum, gradNum);

    for (auto v: gradNum){
        std::cout << v << " ";
    }
    std::cout << std::endl;

    assert((gradNum - grad).cwiseAbs().sum() < 0.000001);
}