#include "00.h"

struct S { };

typedef S SS;

class A
{
  typedef SS SSS;
  SSS s1, s2[10];
};

