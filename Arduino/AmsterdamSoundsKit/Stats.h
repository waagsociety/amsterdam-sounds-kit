/**

 Statistics are computed locally. The devices does not send each measurement, only summaries over time frames (typically one minute).
  
*/

#include "math.h"
#include "inttypes.h"
#include "float.h"

class Stats
{
    double M;
    double S;
    double min;
    double max;
    uint32_t n;
    
public:
    
    Stats()
    {
        Reset();
    }
    
    void Reset()
    {
        n = 0;
        M = 0;
        S = 0;
        min = FLT_MAX;
        max = FLT_MIN;
    }
    
    void Update(double v)
    {
        n += 1;
        double nextM = M + (v - M) / (double)n;
        S = S + (v - M) * (v - nextM);
        M = nextM;
        if(v < min) min = v;
        if(v > max) max = v;
    }

    void UpdateMean(double v)
    {
        n += 1;
        double nextM = M + (v - M) / (double)n;
        S = S + (v - M) * (v - nextM);
        M = nextM;
    }
    
    double Mean()
    {
        return M;
    }
    
    double Min()
    {
        return (min == FLT_MAX ? 0 : min);
    }
    
    double Max()
    {
        return (max == FLT_MIN ? 0 : max);
    }

    uint32_t N()
    {
        return n;
    }
    
    //population or sample variance
    double Variance(bool population)
    {
        if(n < 2) return 0;//NAN;
        return population ? S/(double)n : S/(double)(n-1);
    }
    
    //population or sample variance
    double StdDev(bool population)
    {
        if(n < 2) return 0;//NAN;
        return sqrt(Variance(population));
    }
};
