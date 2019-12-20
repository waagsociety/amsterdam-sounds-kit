/**

 Running average for calculating LEQ over N measurements

*/

#include "math.h"
#include "inttypes.h"
#include "float.h"
#include "SLMSettings.h"

class RunningAvg
{
  uint32_t wi;
  uint32_t filled;
  float buffer[RUNNING_AVG_WINDOW_SIZE];

public:

  RunningAvg()
  {
    Reset();
  }

  void Reset()
  {
    wi = 0;
    filled = 0;
  }

  double Calc()
  {
    double total = 0;

    for (int i = 0; i < filled; i++)
    {
      total += buffer[i];
    }

    return (total/((double)filled));
  }

  double Update(double v)
  {
    buffer[wi] = v;
    if (filled < RUNNING_AVG_WINDOW_SIZE)
    {
      filled++;
    }

    wi = (wi + 1) % RUNNING_AVG_WINDOW_SIZE;
    return (Calc());
  }
};
