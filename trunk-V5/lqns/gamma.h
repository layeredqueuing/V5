/* -*- c++ -*-
 * $HeadURL: svn://franks.dnsalias.com/lqn/trunk/lqns/lqns.h $
 *
 * Gamma distribution.
 *
 * Copyright the Real-Time and Distributed Systems Group,
 * Department of Systems and Computer Engineering,
 * Carleton University, Ottawa, Ontario, Canada. K1S 5B6
 *
 * November, 2012
 *
 * $Id: lqns.h 10906 2012-05-25 12:24:51Z greg $
 *
 * ------------------------------------------------------------------------
 */

#if !defined(LQNS_GAMMA_H)
#define LQNS_GAMMA_H

class Gamma_Distribution
{
public:
    Gamma_Distribution( double shape, double scale );
    double  getDensity(double x) const;
    double getCDF(double x) const;
    
private:
    void setParameters( double shape, double scale );

    static double logGamma(double x);
    static double gammaCDF(double x, double a);
    static double gammaSeries(double x, double a);
    static double gammaCF(double x, double a);
    
    double _shape;
    double _scale;
    double _c;
};
#endif /* LQNS_GAMMA_H */
