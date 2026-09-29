#include "00.h"

class A
{
  forward struct SUnknown;

  [nocode]
  SUnknown s1, s2[10];

  struct SUnknown
  {
  };
};

