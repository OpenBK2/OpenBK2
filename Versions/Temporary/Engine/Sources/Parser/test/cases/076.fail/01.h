#include "00.h"

forward struct SUnknown;

class A
{
  [hidden]
  int n, p;

  [nocode]
  SUnknown s1, s2[10];
};

struct SUnknown
{
};


