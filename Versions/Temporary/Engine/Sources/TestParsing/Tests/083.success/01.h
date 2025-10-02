#include "00.h"

class C { };

class A
{
  enum E { };
  typedef E C;

  C c1[];
};

