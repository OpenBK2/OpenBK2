#include "00.h"

class C { };

typedef C CC;

class A
{
  typedef CC CCC;
  CCC *c;
};

