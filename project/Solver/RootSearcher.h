#ifndef SOLVER_H
#define SOLVER_H

#include <functional>
#include <cassert>
#include <iostream>

double rfbisect(std::function<double(double)> f, double a, double b, double tol)
{
    assert(a < b && f(a) * f(b) < 0);
    double c;
    while( (b-a)/2 > tol ) {
        c = (a+b) / 2.0;
        std::cout << "(a, b) = (" << a << ", " << b << ")" << std::endl;
        if(std::abs(f(c)) < tol)
            return c;
        else {
            if(f(a)*f(c) < 0)
                b = c;
            else
                a = c;
        }
    }
    return c;
}

double rfbrent(std::function<double(double)> f, double a, double b, double tol)
{
    const int ITMAX=100;
    const double EPS=std::numeric_limits<double>::epsilon();

    double c=b,d,e,fa=f(a),fb=f(b); //,fc,p,q,r,s,tol1,xm;
    assert(fa * fb <= 0);
    double fc=fb, p, q, r, s, tol1, xm;
    for (int iter=0;iter<ITMAX;iter++) {
        if ((fb > 0.0 && fc > 0.0) || (fb < 0.0 && fc < 0.0)) {
            c=a;
            fc=fa;
            e=d=b-a;
        }
        if (std::abs(fc) < std::abs(fb)) {
            a=b;
            b=c;
            c=a;
            fa=fb;
            fb=fc;
            fc=fa;
        }
        tol1=2.0*EPS*std::abs(b)+0.5*tol;
        xm=0.5*(c-b);
        if (std::abs(xm) <= tol1 || fb == 0.0) return b;
        if (std::abs(e) >= tol1 && std::abs(fa) > std::abs(fb)) {
            s=fb/fa;
            if (a == c) {
                p=2.0*xm*s;
                q=1.0-s;
            } else {
                q=fa/fc;
                r=fb/fc;
                p=s*(2.0*xm*q*(q-r)-(b-a)*(r-1.0));
                q=(q-1.0)*(r-1.0)*(s-1.0);
            }
            if (p > 0.0) q = -q;
            p=std::abs(p);
            double min1=3.0*xm*q-std::abs(tol1*q);
            double min2=std::abs(e*q);
            if (2.0*p < (min1 < min2 ? min1 : min2)) {
                e=d;
                d=p/q;
            } else {
                d=xm;
                e=d;
            }
        } else {
            d=xm;
            e=d;
        }
        a=b;
        fa=fb;
        if (std::abs(d) > tol1)
            b += d;
        else
            b += xm>0? tol1*xm : tol1*(-xm);
        fb=f(b);
        std::cout << "(a, b) = (" << a << ", " << b << ")" << std::endl;
    }
    throw("Maximum number of iterations exceeded in rfbrent");
}


#endif //SOLVER_H
